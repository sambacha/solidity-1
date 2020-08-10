#include <libsolidity/ast/TypeProvider.h>
#include <libsolidity/boogie/AssignHelper.h>
#include <libsolidity/boogie/ASTBoogieUtils.h>
#include <libsolidity/boogie/BoogieContext.h>
#include <libsolidity/boogie/StoragePtrHelper.h>

using namespace std;
using namespace solidity;
using namespace solidity::frontend;
using namespace langutil;

namespace bg = boogie;

namespace solidity
{
namespace frontend
{


AssignHelper::AssignResult AssignHelper::makeAssign(AssignParam lhs, AssignParam rhs, langutil::Token op,
		ASTNode const* assocNode, BoogieContext& context)
{
	if (lhs.bgExpr->isError() || rhs.bgExpr->isError())
		return AssignResult{};
	AssignResult res;

	// Check if we require the event data to be saved
	if (assocNode)
	{
		for (auto stmt: context.checkForEventDataSave(assocNode))
				res.newStmts.push_back(stmt);
	}

	makeAssignInternal(lhs, rhs, op, assocNode, context, res);
	return res;
}

void AssignHelper::makeAssignInternal(AssignParam lhs, AssignParam rhs, langutil::Token op,
		ASTNode const* assocNode, BoogieContext& context, AssignResult& result)
{
	if (dynamic_cast<TupleType const*>(lhs.type))
	{
		if (dynamic_cast<TupleType const*>(rhs.type))
			makeTupleAssign(lhs, rhs, assocNode, context, result);
		else
			context.reportError(assocNode, "LHS is tuple but RHS is not");
		return;
	}

	if (dynamic_cast<StructType const*>(lhs.type))
	{
		if (dynamic_cast<StructType const*>(rhs.type))
			makeStructAssign(lhs, rhs, assocNode, context, result);
		else
			context.reportError(assocNode, "LHS is struct but RHS is not");
		return;
	}

	if (dynamic_cast<ArrayType const*>(lhs.type))
	{
		if (dynamic_cast<ArrayType const*>(rhs.type) || dynamic_cast<StringLiteralType const*>(rhs.type))
			makeArrayAssign(lhs, rhs, assocNode, context, result);
		else
			context.reportError(assocNode, "LHS is array but RHS is not");
		return;
	}

	// Mappings: only assignment to local storage pointer is valid
	if (dynamic_cast<MappingType const*>(lhs.type))
	{
		if (dynamic_cast<MappingType const*>(rhs.type))
		{
			auto packed = StoragePtrHelper::packToLocalPtr(rhs.expr, rhs.bgExpr, context);
			rhs.bgExpr = packed;
			makeBasicAssign(lhs, rhs, Token::Assign, assocNode, context, result);
		}
		else
			context.reportError(assocNode, "LHS is mapping but RHS is not");
		return;
	}

	makeBasicAssign(lhs, rhs, op, assocNode, context, result);
}

void AssignHelper::makeTupleAssign(AssignParam lhs, AssignParam rhs, ASTNode const* assocNode, BoogieContext& context, AssignResult& result)
{
	auto lhsTuple = dynamic_cast<bg::TupleExpr const*>(lhs.bgExpr.get());
	solAssert(lhsTuple, "Expected tuple as LHS");
	if (context.isBvEncoding())
		rhs.bgExpr = ASTBoogieUtils::checkImplicitBvConversion(rhs.bgExpr, rhs.type, lhs.type, context);
	auto rhsTuple = dynamic_cast<bg::TupleExpr const*>(rhs.bgExpr.get());
	solAssert(rhsTuple, "Expected tuple as RHS");
	auto lhsTupleExpr = dynamic_cast<TupleExpression const*>(lhs.expr);
	auto rhsTupleExpr = dynamic_cast<TupleExpression const*>(rhs.expr);
	auto lhsType = dynamic_cast<TupleType const*>(lhs.type);
	solAssert(lhsType, "Expected tuple type for LHS");
	auto rhsType = dynamic_cast<TupleType const*>(rhs.type);
	solAssert(rhsType, "Expected tuple type for RHS");
	auto const& lhsElems = lhsTuple->elements();
	auto const& rhsElems = rhsTuple->elements();

	// Introduce temp variables due to expressions like (a, b) = (b, a)
	vector<bg::VarDeclRef> tmpVars;
	for (unsigned i = 0; i < lhsElems.size(); ++ i)
	{
		if (lhsElems[i])
		{
			auto rhsElem = rhsTupleExpr ? rhsTupleExpr->components().at(i).get() : nullptr;
			auto rhsElemType = rhsType->components().at(i);
			// Temp variable has the type of LHS by default, so that implicit conversions can happen
			auto tmpVarType = lhsType->components().at(i);
			// For reference types in storage, it should be local storage pointer
			auto rhsRef = dynamic_cast<ReferenceType const*>(rhsElemType);
			if (rhsRef && rhsElemType->dataStoredIn(DataLocation::Storage))
				tmpVarType = TypeProvider::withLocation(rhsRef,DataLocation::Storage, true);

			auto tmp = context.freshTempVar(context.toBoogieType(tmpVarType, assocNode));
			result.newDecls.push_back(tmp);
			tmpVars.push_back(tmp);
			makeAssignInternal(
					AssignParam{tmp->getRefTo(), tmpVarType, nullptr },
					AssignParam{rhsElems[i], rhsElemType, rhsElem },
					Token::Assign, assocNode, context, result);
		}
		else
			tmpVars.push_back(nullptr);
	}

	unsigned i = lhsElems.size();
	solAssert(lhsElem.size() > 0, "Expected non-empty tuple");
	do {
		i --;
		if (lhsElems[i])
		{
			auto const lhsElem = lhsTupleExpr ? lhsTupleExpr->components().at(i).get() : nullptr;
			auto lhsElemType = lhsType->components().at(i);
			auto rhsElemType = rhsType->components().at(i);
			// Temp variable has the type of LHS by default
			auto tmpVarType = lhsElemType;
			// For reference types in storage, it should be local storage pointer
			auto rhsRef = dynamic_cast<ReferenceType const*>(rhsElemType);
			if (rhsRef && rhsElemType->dataStoredIn(DataLocation::Storage))
				tmpVarType = TypeProvider::withLocation(rhsRef, DataLocation::Storage, true);

			auto rhsElem = rhsTupleExpr ? rhsTupleExpr->components().at(i).get() : nullptr;
			makeAssignInternal(
					AssignParam{lhsElems[i], lhsElemType, lhsElem },
					AssignParam{tmpVars[i]->getRefTo(), tmpVarType, rhsElem },
					Token::Assign, assocNode, context, result);
		}
	} while (i != 0);
}

void AssignHelper::makeStructAssign(AssignParam lhs, AssignParam rhs, ASTNode const* assocNode, BoogieContext& context, AssignResult& result)
{
	auto lhsType = dynamic_cast<StructType const*>(lhs.type);
	auto rhsType = dynamic_cast<StructType const*>(rhs.type);
	solAssert(lhsType, "Expected struct type for LHS");
	solAssert(rhsType, "Expected struct type for RHS");
	auto lhsLoc = lhsType->location();
	auto rhsLoc = rhsType->location();
	// LHS is memory
	if (lhsLoc == DataLocation::Memory)
	{
		// RHS is memory --> reference copy
		if (rhsLoc == DataLocation::Memory)
		{
			makeBasicAssign(lhs, rhs, Token::Assign, assocNode, context, result);
			return;
		}
		// RHS is storage or calldata --> create new, deep copy
		else if (rhsLoc == DataLocation::Storage || rhsLoc == DataLocation::CallData)
		{
			// Create new
			auto allocRes = ASTBoogieUtils::newStruct(&lhsType->structDefinition(), context);
			result.newDecls.push_back(allocRes.newDecl);
			for (auto stmt: allocRes.newStmts)
				result.newStmts.push_back(stmt);
			result.newStmts.push_back(bg::Stmt::assign(lhs.bgExpr, allocRes.newDecl->getRefTo()));

			// RHS is local storage: unpack first
			if (rhsLoc == DataLocation::Storage && rhsType->isPointer())
				rhs.bgExpr = StoragePtrHelper::unpackLocalPtr(rhs.expr, rhs.bgExpr, context);

			// Make deep copy
			deepCopyStruct(&lhsType->structDefinition(), lhs.bgExpr, rhs.bgExpr,
					lhsLoc, rhsLoc, assocNode, context, result);
			return;
		}
		else
		{
			context.reportError(assocNode, "Assignment from " +
					ASTBoogieUtils::dataLocToStr(rhsLoc) +
					" to memory struct is not supported");
			return;
		}
	}
	// LHS is storage
	else if (lhsLoc == DataLocation::Storage)
	{
		// RHS is storage
		if (rhsLoc == DataLocation::Storage)
		{
			// LHS is storage pointer --> reference copy
			if (lhsType->isPointer())
			{
				// pointer to pointer, simply assign
				if (rhsType->isPointer())
				{
					makeBasicAssign(lhs, rhs, Token::Assign, assocNode, context, result);
					return;
				}
				else // Otherwise pack
				{
					auto packed = StoragePtrHelper::packToLocalPtr(rhs.expr, rhs.bgExpr, context);
					result.newStmts.push_back(bg::Stmt::assign(lhs.bgExpr, packed));
					return;
				}
			}
			// LHS is storage --> deep copy by data types
			else
			{
				// Unpack pointers first
				if (rhsType->isPointer())
					rhs.bgExpr = StoragePtrHelper::unpackLocalPtr(rhs.expr, rhs.bgExpr, context);

				makeBasicAssign(lhs, rhs, Token::Assign, assocNode, context, result);
				return;
			}
		}
		// RHS is memory --> deep copy
		else if (rhsLoc == DataLocation::Memory)
		{
			deepCopyStruct(&lhsType->structDefinition(), lhs.bgExpr, rhs.bgExpr,
					lhsLoc, rhsLoc, assocNode, context, result);
			return;
		}
		else
		{
			context.reportError(assocNode, "Assignment from " +
					ASTBoogieUtils::dataLocToStr(rhsLoc) + " to storage struct is not supported");
			return;
		}
	}
	context.reportError(assocNode, "Assigning to " +
			ASTBoogieUtils::dataLocToStr(lhsLoc) + " struct is not supported");
}

void AssignHelper::makeArrayAssign(AssignParam lhs, AssignParam rhs, ASTNode const* assocNode, BoogieContext& context, AssignResult& result)
{
	// We can have arrays on LHS but RHS can be a string literal
	auto lhsType = dynamic_cast<ArrayType const*>(lhs.type);
	auto rhsTypeArray = dynamic_cast<ArrayType const*>(rhs.type);
	auto rhsTypeStringLiteral = dynamic_cast<StringLiteralType const*>(rhs.type);
	solAssert(lhsType, "Expected array type for LHS");
	solAssert(rhsTypeArray || rhsTypeStringLiteral, "Expected array type for RHS");

	DataLocation lhsLocation = lhsType->location();
	DataLocation rhsLocation = rhsTypeArray ? rhsTypeArray->location() : DataLocation::Storage;

	if (lhsLocation != rhsLocation && rhsTypeArray &&
			dynamic_cast<ReferenceType const*>(rhsTypeArray->baseType()))
	{
		context.reportError(assocNode, "Deep copy between storage/memory arrays with reference types is not yet supported.");
	}

	// Check if locations are compatible
	if (lhsLocation != rhsLocation)
	{
		if (lhsLocation == DataLocation::Memory)
		{
			// Create new
			auto allocRes = ASTBoogieUtils::newArray(context.toBoogieType(lhsType, assocNode), context);
			result.newDecls.push_back(allocRes.newDecl);
			for (auto stmt: allocRes.newStmts)
				result.newStmts.push_back(stmt);
			result.newStmts.push_back(bg::Stmt::assign(lhs.bgExpr, allocRes.newDecl->getRefTo()));
			lhs.bgExpr = context.getMemArray(lhs.bgExpr, context.toBoogieType(lhsType->baseType(), assocNode));
		}
		if (rhsLocation == DataLocation::Memory || rhsLocation == DataLocation::CallData)
			rhs.bgExpr = context.getMemArray(rhs.bgExpr, context.toBoogieType(rhsTypeArray->baseType(), assocNode));
		if (rhsTypeArray && rhsLocation == DataLocation::Storage && rhsTypeArray->isPointer())
			rhs.bgExpr = StoragePtrHelper::unpackLocalPtr(rhs.expr, rhs.bgExpr, context);
	}
	else if (lhsLocation == DataLocation::Storage) // If locations are same
	{
		// Pack into local pointer (reassigning local pointer)
		if (lhsType->isPointer() && rhsTypeArray && !rhsTypeArray->isPointer())
		{
			auto packed = StoragePtrHelper::packToLocalPtr(rhs.expr, rhs.bgExpr, context);
			rhs.bgExpr = packed;
		}
		else if (!lhsType->isPointer() && rhsTypeArray && rhsTypeArray->isPointer())
		{
			// Unpack local pointer when assigning to storage
			rhs.bgExpr = StoragePtrHelper::unpackLocalPtr(rhs.expr, rhs.bgExpr, context);
		}
	}

	makeBasicAssign(lhs, rhs, Token::Assign, assocNode, context, result);
}

void AssignHelper::makeBasicAssign(AssignParam lhs, AssignParam rhs, langutil::Token op, ASTNode const* assocNode, BoogieContext& context, AssignResult& result)
{
	// Bit-precise mode
	if (context.isBvEncoding() && ASTBoogieUtils::isBitPreciseType(lhs.type))
		// Check for implicit conversion
		rhs.bgExpr = ASTBoogieUtils::checkImplicitBvConversion(rhs.bgExpr, rhs.type, lhs.type, context);

	ASTBoogieUtils::ExprWithCC rhsResult;
	// Check for additional arithmetic needed
	if (op == Token::Assign)
	{
		rhsResult.expr = rhs.bgExpr; // rhs already contains the result
	}
	else
	{
		// Transform rhs based on the operator, e.g., a += b becomes a := (a + b)
		unsigned bits = ASTBoogieUtils::getBits(lhs.type);
		bool isSigned = ASTBoogieUtils::isSigned(lhs.type);
		rhsResult = ASTBoogieUtils::encodeArithBinaryOp(context, assocNode, op, lhs.bgExpr, rhs.bgExpr, bits, isSigned);
	}
	if (context.overflow() && rhsResult.cc)
		result.ocs.push_back(rhsResult.cc);

	// First lift conditionals (due to local pointers)
	lhs.bgExpr = liftCond(lhs.bgExpr);
	// Then check if sum ghost variables need to be updated
	for (auto stmt: checkForSums(lhs.bgExpr, rhsResult.expr, context))
		result.newStmts.push_back(stmt);

	result.newStmts.push_back(bg::Stmt::assign(lhs.bgExpr, rhsResult.expr));
}

boogie::Expr::Ref AssignHelper::liftCond(boogie::Expr::Ref expr)
{
	if (auto selExpr = dynamic_pointer_cast<bg::SelExpr const>(expr))
	{
		// First recurse
		selExpr = dynamic_pointer_cast<bg::SelExpr const>(selExpr->replaceBase(liftCond(selExpr->getBase())));
		solAssert(selExpr, "");
		// Then check if lifting is needed
		if (auto baseCond = dynamic_pointer_cast<bg::CondExpr const>(selExpr->getBase()))
		{
			return bg::Expr::cond(baseCond->getCond(),
					liftCond(selExpr->replaceBase(baseCond->getThen())),
					liftCond(selExpr->replaceBase(baseCond->getElse())));
		}
		return selExpr;
	}
	return expr;
}

list<bg::Stmt::Ref> AssignHelper::checkForSums(bg::Expr::Ref lhs, bg::Expr::Ref rhs, BoogieContext& context)
{
	if (auto condExpr = dynamic_pointer_cast<bg::CondExpr const>(lhs))
	{
		vector<bg::Stmt::Ref> thenSums;
		for (auto stmt: checkForSums(condExpr->getThen(), rhs, context))
			thenSums.push_back(stmt);
		vector<bg::Stmt::Ref> elseSums;
		for (auto stmt: checkForSums(condExpr->getElse(), rhs, context))
			elseSums.push_back(stmt);

		if (thenSums.empty() && elseSums.empty())
			return {};
		else
			return {bg::Stmt::ifelse(condExpr->getCond(), bg::Block::block("", thenSums), bg::Block::block("", elseSums))};
	}
	return context.updateSumVars(lhs, rhs);
}

void AssignHelper::deepCopyStruct(StructDefinition const* structDef,
		bg::Expr::Ref lhsBase, bg::Expr::Ref rhsBase, DataLocation lhsLoc, DataLocation rhsLoc,
		ASTNode const* assocNode, BoogieContext& context, AssignResult& result)
{
	result.newStmts.push_back(bg::Stmt::comment("Deep copy struct " + structDef->name()));
	// Loop through each member
	for (auto member: structDef->members())
	{
		// Get expressions for accessing members
		bg::Expr::Ref lhsSel = nullptr;
		if (lhsLoc == DataLocation::Storage)
			lhsSel = bg::Expr::dtsel(lhsBase, context.mapDeclName(*member),
					context.getStructConstructor(structDef),
					dynamic_pointer_cast<bg::DataTypeDecl>(context.getStructType(structDef, lhsLoc)));
		else
			lhsSel = bg::Expr::arrsel(bg::Expr::id(context.mapDeclName(*member)), lhsBase);

		bg::Expr::Ref rhsSel = nullptr;
		if (rhsLoc == DataLocation::Storage)
			rhsSel = bg::Expr::dtsel(rhsBase, context.mapDeclName(*member),
					context.getStructConstructor(structDef),
					dynamic_pointer_cast<bg::DataTypeDecl>(context.getStructType(structDef, rhsLoc)));
		else
			rhsSel = bg::Expr::arrsel(bg::Expr::id(context.mapDeclName(*member)), rhsBase);

		auto memberType = member->annotation().type;
		auto memberTypeCat = memberType->category();
		// For nested structs do recursion
		if (memberTypeCat == Type::Category::Struct)
		{
			auto memberStructType = dynamic_cast<StructType const*>(memberType);
			// Deep copy into memory creates new
			if (lhsLoc == DataLocation::Memory)
			{
				// Create new
				auto allocRes = ASTBoogieUtils::newStruct(&memberStructType->structDefinition(), context);
				result.newDecls.push_back(allocRes.newDecl);
				for (auto stmt: allocRes.newStmts)
					result.newStmts.push_back(stmt);
				// Update member to point to new
				makeBasicAssign(
						AssignParam{lhsSel, memberType, nullptr},
						AssignParam{allocRes.newDecl->getRefTo(), memberType, nullptr},
						Token::Assign, assocNode, context, result);
			}
			// Do the deep copy
			deepCopyStruct(&memberStructType->structDefinition(), lhsSel, rhsSel, lhsLoc, rhsLoc, assocNode, context, result);
		}
		else if (memberTypeCat == Type::Category::Mapping)
		{
			// Mappings are simply skipped
		}
		else if (memberTypeCat == Type::Category::Array)
		{
			auto arrType = dynamic_cast<ArrayType const*>(memberType);
			if (lhsLoc == DataLocation::Memory)
			{
				// Create new
				auto allocRes = ASTBoogieUtils::newArray(
						context.toBoogieType(TypeProvider::withLocation(arrType, DataLocation::Memory, false), assocNode),
						context);
				result.newDecls.push_back(allocRes.newDecl);
				for (auto stmt: allocRes.newStmts)
					result.newStmts.push_back(stmt);
				// Update member to point to new
				makeBasicAssign(
						AssignParam{lhsSel, memberType, nullptr},
						AssignParam{allocRes.newDecl->getRefTo(), memberType, nullptr},
						Token::Assign, assocNode, context, result);
				// The LHS becomes the global memory array, RHS should be copied here
				lhsSel = context.getMemArray(allocRes.newDecl->getRefTo(),
						context.toBoogieType(arrType->baseType(), assocNode));
			}
			if (rhsLoc == DataLocation::Memory || rhsLoc == DataLocation::CallData)
				rhsSel = context.getMemArray(rhsSel, context.toBoogieType(arrType->baseType(), assocNode));

			makeBasicAssign(
					AssignParam{lhsSel, memberType, nullptr},
					AssignParam{rhsSel, memberType, nullptr},
					Token::Assign, assocNode, context, result);
		}
		// For other types make the copy by updating the LHS with RHS
		else
		{
			makeBasicAssign(
					AssignParam{lhsSel, memberType, nullptr},
					AssignParam{rhsSel, memberType, nullptr},
					Token::Assign, assocNode, context, result);
		}
	}
}

}

}
