#include <libsolidity/boogie/ASTBoogieUtils.h>
#include <libsolidity/boogie/BoogieContext.h>
#include <libsolidity/boogie/StoragePtrHelper.h>

using namespace std;
using namespace dev;
using namespace dev::solidity;
using namespace langutil;

namespace bg = boogie;

namespace dev
{
namespace solidity
{
bg::Expr::Ref StoragePtrHelper::packToLocalPtr(Expression const* expr, bg::Expr::Ref bgExpr, BoogieContext& context)
{
	if (!dynamic_cast<StructType const*>(expr->annotation().type) &&
		!dynamic_cast<ArrayType const*>(expr->annotation().type) &&
		!dynamic_cast<MappingType const*>(expr->annotation().type))
	{
		context.reportError(expr, "Expected array, mapping or struct type");
		return bg::Expr::error();
	}
	// Function calls return pointers, no need to pack, just copy the return value
	if (dynamic_cast<FunctionCall const*>(expr))
		return bgExpr;

	PackResult packed {{}, {}};
	packInternal(expr, bgExpr, context, packed);

	// Single array: convert to array write and return
	if (packed.exprs.size() == 1)
		return toWriteExpr(packed.exprs[0], context);

	// Multiple arrays: if-then-else of array writes
	if (packed.exprs.size() > 1)
	{
		solAssert(packed.exprs.size() == packed.conds.size(), "Expression and condition mismatch");
		// Start with last
		bg::Expr::Ref result = toWriteExpr(packed.exprs[packed.exprs.size() - 1], context);
		// Create if-then-else recursively using the rest
		for (int i = packed.exprs.size() - 2; i >= 0; --i)
			result = bg::Expr::cond(packed.conds[i], toWriteExpr(packed.exprs[i], context), result);
		return result;
	}

	context.reportError(expr, "Unsupported expression for packing into local pointer");
	return bg::Expr::error();
}

bg::Expr::Ref StoragePtrHelper::toWriteExpr(vector<bg::Expr::Ref> exprs, BoogieContext& context)
{
	// Start with a constant [int]int array with default value of 0
	auto intType = context.intType(256);
	string smtExpr = "((as const (Array " + intType->getSmtType() + " " + intType->getSmtType() + ")) " +
			context.intLit(bg::bigint(0), 256)->toSMT2() + ")";
	bg::Expr::Ref write = bg::Expr::fn(context.defaultArray(intType, intType, smtExpr)->getName(), vector<bg::Expr::Ref>());
	// Then do a series of writes into the array
	for (size_t i = 0; i < exprs.size(); ++i)
		write = bg::Expr::arrupd(write, context.intLit(bg::bigint(i), 256), exprs[i]);
	return write;
}

void StoragePtrHelper::packInternal(Expression const* expr, bg::Expr::Ref bgExpr, BoogieContext& context, PackResult& result)
{
	// Packs an expression (path to storage) into a local pointer as an array
	// The general idea is to associate each state variable and member with an index
	// so that the path can be encoded as an integer array.
	//
	// Example:
	// contract C {
	//   struct T { int z; }
	//   struct S {
	//     T t1;     --> 0
	//     T[] ts;   --> 1 + array index
	//   }
	//
	//   T t1;       --> 0
	//   S s1;       --> 1
	//   S[] ss;     --> 2 + array index
	// }
	//
	// t1 becomes [0]
	// s1.t1 becomes [1, 0]
	// ss[5].ts[3] becomes [2, 5, 1, 3]

	// Identifier: search for matching state variable in the contract
	if (auto idExpr = dynamic_cast<Identifier const*>(expr))
	{
		// Must be the first element in the packed array
		solAssert(result.conds.empty(), "Non-empty expressions when packing identifier");
		solAssert(result.exprs.empty(), "Non-empty expressions when packing identifier");

		// Collect all variables from all contracts
		vector<VariableDeclaration const*> vars;
		for (auto contr: context.stats().allContracts())
		{
			auto subVars = ASTNode::filteredNodes<VariableDeclaration>(contr->subNodes());
			vars.insert(vars.end(), subVars.begin(), subVars.end());
		}
		for (unsigned i = 0; i < vars.size(); ++i)
		{
			// If found, initialize packed array with its index
			if (vars[i] == idExpr->annotation().referencedDeclaration)
			{
				result.conds.push_back(bg::Expr::lit(true));
				result.exprs.push_back({context.intLit(bg::bigint(i), 256)});
				return;
			}
		}

		// Identifier points to local storage pointer: repack
		if (auto refType = dynamic_cast<ReferenceType const*>(idExpr->annotation().type))
		{
			if (refType->dataStoredIn(DataLocation::Storage) && refType->isPointer())
			{
				auto repacked = repack(
						idExpr,
						bg::Expr::id(context.mapDeclName(*idExpr->annotation().referencedDeclaration)),
						context.currentContract(), 0, context);
				for (auto c: repacked.conds)
					result.conds.push_back(c);
				for (auto e: repacked.exprs)
					result.exprs.push_back(e);
				return;
			}
		}

		context.reportError(expr, "Unsupported identifier as base of local storage pointer");
		result.conds.push_back(bg::Expr::error());
		result.exprs.push_back({bg::Expr::error()});
		return;
	}
	// Member access: process base recursively, then find matching member
	else if (auto memAccExpr = dynamic_cast<MemberAccess const*>(expr))
	{
		auto bgMemAccExpr = dynamic_pointer_cast<bg::DtSelExpr const>(bgExpr);
		solAssert(bgMemAccExpr, "Expected Boogie member access expression");
		packInternal(&memAccExpr->expression(), bgMemAccExpr->getBase(), context, result);
		solAssert(!result.exprs.empty(), "Empty pointer from subexpression");
		auto eStructType = dynamic_cast<StructType const*>(memAccExpr->expression().annotation().type);
		solAssert(eStructType, "Trying to pack member of non-struct expression");
		auto members = eStructType->structDefinition().members();
		for (unsigned i = 0; i < members.size(); ++i)
		{
			// If matching member found, put index in next position of each packed array
			if (members[i].get() == memAccExpr->annotation().referencedDeclaration)
			{
				for (size_t e = 0; e < result.exprs.size(); ++e)
					result.exprs[e].push_back(context.intLit(bg::bigint(i), 256));
				return;
			}
		}
	}
	// Arrays and mappings: process base recursively, then store index expression in next position
	// in the packed array
	else if (auto idxExpr = dynamic_cast<IndexAccess const*>(expr))
	{
		auto bgIdxAccExpr = dynamic_pointer_cast<bg::ArrSelExpr const>(bgExpr);
		solAssert(bgIdxAccExpr, "Expected Boogie index access expression");
		bg::Expr::Ref base = bgIdxAccExpr->getBase();

		// Base has to be extracted in two steps for arrays, because an array is
		// actually a datatype with the actual array and its length
		if (idxExpr->baseExpression().annotation().type->category() == Type::Category::Array)
		{
			auto bgDtSelExpr = dynamic_pointer_cast<bg::DtSelExpr const>(bgIdxAccExpr->getBase());
			solAssert(bgIdxAccExpr, "Expected Boogie member access expression below indexer");
			base = bgDtSelExpr->getBase();
		}

		// Report error for unsupported index types
		auto idxType = idxExpr->indexExpression()->annotation().type->category();
		if (idxType == Type::Category::StringLiteral || idxType == Type::Category::Array)
		{
			context.reportError(idxExpr->indexExpression(), "Unsupported type for index in local pointer");
			return;
		}

		packInternal(&idxExpr->baseExpression(), base, context, result);
		solAssert(!result.exprs.empty(), "Empty pointer from subexpression");

		// Append index into each packed array
		for (size_t e = 0; e < result.exprs.size(); ++e)
			result.exprs[e].push_back(bgIdxAccExpr->getIdx());
		return;
	}

	context.reportError(expr, "Unsupported expression for packing into local pointer");
}

bg::Expr::Ref StoragePtrHelper::unpackLocalPtr(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, BoogieContext& context)
{
	auto result = unpackInternal(ptrExpr, ptrBgExpr, context.currentContract(), 0, nullptr, context);
	if (!result)
		context.reportError(ptrExpr, "Nothing to unpack, perhaps there are no instances of the type");
	return result;
}

bg::Expr::Ref StoragePtrHelper::unpackInternal(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, Declaration const* decl, int depth, bg::Expr::Ref base, BoogieContext& context)
{
	// Unpacks a local storage pointer represented as an array of integers
	// into a conditional expression. The general idea is the opposite of packing:
	// go through each state variable (recursively for complex types) and associate
	// a conditional expression. For the example in pack, unpacking an array [arr]
	// would be like the following:
	// ite(arr[0] == 0, t1,
	// ite(arr[0] == 1,
	//   ite(arr[1] == 0, s1.t1, s1.ts[arr[2]], ... )))

	// Contract: go through state vars and create conditional expression recursively
	if (dynamic_cast<ContractDefinition const*>(decl))
	{
		auto structType = dynamic_cast<StructType const*>(ptrExpr->annotation().type);
		auto arrayType = dynamic_cast<ArrayType const*>(ptrExpr->annotation().type);
		auto mapType = dynamic_cast<MappingType const*>(ptrExpr->annotation().type);
		solAssert(structType || arrayType || mapType, "Expected array, mapping or struct type when unpacking");
		// Collect all variables from all contracts that can see the struct
		vector<VariableDeclaration const*> vars;
		for (auto contr: context.stats().allContracts())
		{
			auto subVars = ASTNode::filteredNodes<VariableDeclaration>(contr->subNodes());
			vars.insert(vars.end(), subVars.begin(), subVars.end());
		}

		// Default context (if no state var to unpack to)
		bg::Expr::Ref unpackedExpr = nullptr;
		if (structType)
			unpackedExpr = bg::Expr::arrsel(context.getDefaultStorageContext(structType)->getRefTo(),
					bg::Expr::arrsel(ptrBgExpr, context.intLit(1, 256)));
		else if (arrayType)
			unpackedExpr = bg::Expr::arrsel(context.getDefaultStorageContext(arrayType)->getRefTo(),
				bg::Expr::arrsel(ptrBgExpr, context.intLit(1, 256)));
		for (unsigned i = 0; i < vars.size(); ++i)
		{
			auto subExpr = unpackInternal(ptrExpr, ptrBgExpr, vars[i], depth+1,
					bg::Expr::arrsel(bg::Expr::id(context.mapDeclName(*vars[i])), context.boogieThis()->getRefTo()), context);
			if (subExpr)
			{
				if (unpackedExpr)
					unpackedExpr = bg::Expr::cond(
							bg::Expr::eq(bg::Expr::arrsel(ptrBgExpr, context.intLit(depth, 256)), context.intLit(i, 256)),
							subExpr, unpackedExpr);
				else
					unpackedExpr = subExpr;
			}
		}
		return unpackedExpr;
	}
	// Variable (state var or struct member)
	else if (auto varDecl = dynamic_cast<VariableDeclaration const*>(decl))
	{
		auto targetStructTp = dynamic_cast<StructType const*>(ptrExpr->annotation().type);
		auto targetArrayTp = dynamic_cast<ArrayType const*>(ptrExpr->annotation().type);
		auto targetMapTp = dynamic_cast<MappingType const*>(ptrExpr->annotation().type);
		solAssert(targetStructTp || targetArrayTp || targetMapTp, "Expected array, mapping or struct type when unpacking");
		auto declTp = varDecl->type();

		// Get rid of arrays and mappings by indexing into them
		while (declTp->category() == Type::Category::Array || declTp->category() == Type::Category::Mapping)
		{
			if (auto arrType = dynamic_cast<ArrayType const*>(declTp))
			{
				// Found a variable with a matching type, just return
				if (targetArrayTp && targetArrayTp->isImplicitlyConvertibleTo(*arrType))
					return base;
				auto bgType = context.toBoogieType(arrType->baseType(), ptrExpr);
				base = bg::Expr::arrsel(
						context.getInnerArray(base, context.toBoogieType(arrType->baseType(), ptrExpr)),
						bg::Expr::arrsel(ptrBgExpr, context.intLit(depth, 256)));
				declTp = arrType->baseType();
			}
			else if (auto mapType = dynamic_cast<MappingType const*>(declTp))
			{
				// Found a variable with a matching type, just return
				if (targetMapTp && targetMapTp->isImplicitlyConvertibleTo(*mapType))
					return base;
				base = bg::Expr::arrsel(base, bg::Expr::arrsel(ptrBgExpr, context.intLit(depth, 256)));
				declTp = mapType->valueType();
			}
			else
				solAssert(false, "Expected array or mapping type");
			depth++;
		}

		auto declStructTp = dynamic_cast<StructType const*>(declTp);

		// Found a variable with a matching type, just return
		if (targetStructTp && declStructTp && targetStructTp->structDefinition() == declStructTp->structDefinition())
		{
			return base;
		}
		// Otherwise if it is a struct, go through members and recurse
		if (declStructTp)
		{
			auto members = declStructTp->structDefinition().members();
			bg::Expr::Ref expr = nullptr;
			for (unsigned i = 0; i < members.size(); ++i)
			{
				auto baseForSub = bg::Expr::dtsel(base,
						context.mapDeclName(*members[i]),
						context.getStructConstructor(&declStructTp->structDefinition()),
						dynamic_pointer_cast<bg::DataTypeDecl>(context.getStructType(&declStructTp->structDefinition(), DataLocation::Storage)));
				auto subExpr = unpackInternal(ptrExpr, ptrBgExpr, members[i].get(), depth+1, baseForSub, context);
				if (subExpr)
				{
					if (!expr)
						expr = subExpr;
					else
						expr = bg::Expr::cond(
								bg::Expr::eq(bg::Expr::arrsel(ptrBgExpr, context.intLit(depth, 256)), context.intLit(i, 256)),
								subExpr, expr);
				}
			}
			return expr;
		}
	}
	return nullptr;
}

StoragePtrHelper::PackResult StoragePtrHelper::repack(Expression const* ptrExpr, bg::Expr::Ref ptrBgExpr, Declaration const* decl, int depth, BoogieContext& context)
{
	if (dynamic_cast<ContractDefinition const*>(decl))
	{
		auto structType = dynamic_cast<StructType const*>(ptrExpr->annotation().type);
		auto arrayType = dynamic_cast<ArrayType const*>(ptrExpr->annotation().type);
		auto mapType = dynamic_cast<MappingType const*>(ptrExpr->annotation().type);
		solAssert(structType || arrayType || mapType, "Expected array, mapping or struct type when unpacking");
		// Collect all variables from all contracts
		vector<VariableDeclaration const*> vars;
		for (auto contr: context.stats().allContracts())
		{
			auto subVars = ASTNode::filteredNodes<VariableDeclaration>(contr->subNodes());
			vars.insert(vars.end(), subVars.begin(), subVars.end());
		}
		PackResult repacked = {{}, {}};
		for (unsigned i = 0; i < vars.size(); ++i)
		{
			PackResult sub = repack(ptrExpr, ptrBgExpr, vars[i], depth+1, context);
			if (!sub.exprs.empty())
			{
				for (size_t s = 0; s < sub.exprs.size(); ++s)
				{
					bg::Expr::Ref cond = bg::Expr::and_(
							bg::Expr::eq(
								bg::Expr::arrsel(ptrBgExpr, context.intLit(bg::bigint(depth), 256)),
								context.intLit(bg::bigint(i), 256)),
							sub.conds[s]);
					repacked.conds.push_back(cond);
					sub.exprs[s].insert(sub.exprs[s].begin(), context.intLit(bg::bigint(i), 256));
					repacked.exprs.push_back(sub.exprs[s]);
				}
			}
		}
		return repacked;
	}
	// Variable (state var or struct member)
	else if (auto varDecl = dynamic_cast<VariableDeclaration const*>(decl))
	{
		auto targetStructTp = dynamic_cast<StructType const*>(ptrExpr->annotation().type);
		auto targetArrayTp = dynamic_cast<ArrayType const*>(ptrExpr->annotation().type);
		auto targetMapTp = dynamic_cast<MappingType const*>(ptrExpr->annotation().type);
		solAssert(targetStructTp || targetArrayTp || targetMapTp, "Expected array, mapping or struct type when unpacking");
		auto declTp = varDecl->type();

		vector<bg::Expr::Ref> indices;

		// Get rid of arrays and mappings by indexing into them
		while (declTp->category() == Type::Category::Array || declTp->category() == Type::Category::Mapping)
		{
			if (auto arrType = dynamic_cast<ArrayType const*>(declTp))
			{
				// Found a variable with a matching type, just return
				if (targetArrayTp && targetArrayTp->isImplicitlyConvertibleTo(*arrType))
				{
					PackResult repacked;
					repacked.conds.push_back(bg::Expr::lit(true));
					repacked.exprs.push_back(indices);
					return repacked;
				}
				indices.push_back(bg::Expr::arrsel(ptrBgExpr, context.intLit(bg::bigint(depth), 256)));
				declTp = arrType->baseType();
			}

			else if (auto mapType = dynamic_cast<MappingType const*>(declTp))
			{
				// Found a variable with a matching type, just return
				if (targetMapTp && targetMapTp->isImplicitlyConvertibleTo(*mapType))
				{
					PackResult repacked;
					repacked.conds.push_back(bg::Expr::lit(true));
					repacked.exprs.push_back(indices);
					return repacked;
				}
				indices.push_back(bg::Expr::arrsel(ptrBgExpr, context.intLit(bg::bigint(depth), 256)));
				declTp = mapType->valueType();
			}
			else
				solAssert(false, "Expected array or mapping type");
			depth++;
		}

		auto declStructTp = dynamic_cast<StructType const*>(declTp);

		// Found a variable with a matching type, just return
		if (targetStructTp && declStructTp && targetStructTp->structDefinition() == declStructTp->structDefinition())
		{
			PackResult repacked;
			repacked.conds.push_back(bg::Expr::lit(true));
			repacked.exprs.push_back(indices);
			return repacked;
		}

		// Otherwise if it is a struct, go through members and recurse
		if (declStructTp)
		{
			PackResult repacked;
			auto members = declStructTp->structDefinition().members();
			for (unsigned i = 0; i < members.size(); ++i)
			{
				PackResult sub = repack(ptrExpr, ptrBgExpr, members[i].get(), depth+1, context);
				if (!sub.exprs.empty())
				{
					for (size_t s = 0; s < sub.exprs.size(); ++s)
					{
						bg::Expr::Ref cond = bg::Expr::and_(
								bg::Expr::eq(
									bg::Expr::arrsel(ptrBgExpr, context.intLit(bg::bigint(depth), 256)),
									context.intLit(bg::bigint(i), 256)),
								sub.conds[s]);
						repacked.conds.push_back(cond);
						sub.exprs[s].insert(sub.exprs[s].begin(), indices.begin(), indices.end());
						sub.exprs[s].insert(sub.exprs[s].begin(), context.intLit(bg::bigint(i), 256));
						repacked.exprs.push_back(sub.exprs[s]);
					}
				}
			}
			return repacked;
		}

		return PackResult{{},{}};
	}
	// TODO
	return PackResult{{},{}};
}

}

}
