#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/TypeProvider.h>
#include <libsolidity/boogie/ASTBoogieUtils.h>
#include <libsolidity/boogie/BoogieContext.h>
#include <libsolidity/boogie/BoogieAst.h>

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/SourceReferenceFormatter.h>

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace solidity::langutil;

namespace bg = boogie;

namespace solidity::frontend
{

/// Magic variables get negative ids for easy differentiation
int BoogieContext::BoogieGlobalContext::newMagicVariableID()
{
	return m_nextMagicVariableId --;
}

BoogieContext::BoogieGlobalContext::BoogieGlobalContext()
:
		m_nextMagicVariableId(-100)
{
	// Remove all variables, so we can just add our own
	m_magicVariables.clear();

	// Add magic variables for the 'sum' function for all sizes of int and uint
	for (string sumType: { "int", "uint" })
	{
		auto funType = TypeProvider::function(strings { }, strings { sumType },
				FunctionType::Kind::Internal, true, StateMutability::Pure);
		auto funName = ASTBoogieUtils::VERIFIER_SUM + "_" + sumType;
		auto sum = new MagicVariableDeclaration(newMagicVariableID(), funName, funType);
		m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(sum));
	}

	// Add magic variables for the 'old' function
	for (string oldType: { "address", "bool", "int", "uint" })
	{
		auto funType = TypeProvider::function(strings { oldType }, strings { oldType },
				FunctionType::Kind::Internal, false, StateMutability::Pure);
		auto funName = ASTBoogieUtils::VERIFIER_OLD + "_" + oldType;
		auto old = new MagicVariableDeclaration(newMagicVariableID(), funName, funType);
		m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(old));
	}

	// Add magic variables for the 'before' function
	for (string befType: { "address", "bool", "int", "uint" })
	{
		auto funType = TypeProvider::function(strings { befType }, strings { befType },
				FunctionType::Kind::Internal, false, StateMutability::Pure);
		auto funName = ASTBoogieUtils::VERIFIER_BEFORE + "_" + befType;
		auto bef = new MagicVariableDeclaration(newMagicVariableID(), funName, funType);
		m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(bef));
	}

	// Magic variable for 'eq' function
	auto eqFunType = TypeProvider::function(strings { }, strings { "bool" },
					FunctionType::Kind::Internal, true, StateMutability::Pure);
	auto eq = new MagicVariableDeclaration(newMagicVariableID(), ASTBoogieUtils::VERIFIER_EQ, eqFunType);
	m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(eq));

	// Add magic variables for 'index'
	m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(
			new MagicVariableDeclaration(newMagicVariableID(), ASTBoogieUtils::VERIFIER_IDX + "_address",
					TypeProvider::address())));
	m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(
			new MagicVariableDeclaration(newMagicVariableID(), ASTBoogieUtils::VERIFIER_IDX + "_int",
					TypeProvider::integer(8, IntegerType::Modifier::Signed))));
	m_magicVariables.push_back(shared_ptr<MagicVariableDeclaration const>(
			new MagicVariableDeclaration(newMagicVariableID(), ASTBoogieUtils::VERIFIER_IDX + "_uint",
					TypeProvider::integer(8, IntegerType::Modifier::Unsigned))));
}

BoogieContext::BoogieContext(Encoding encoding,
		bool overflow,
		bool modAnalysis,
		ErrorReporter* errorReporter,
		std::map<ASTNode const*,
		std::shared_ptr<DeclarationContainer>> scopes,
		EVMVersion evmVersion,
		ASTBoogieStats const& stats)
:
		m_stats(stats), m_program(), m_encoding(encoding), m_overflow(overflow),
		m_modAnalysis(modAnalysis), m_errorReporter(errorReporter),
		m_currentScanner(nullptr), m_scopes(scopes), m_evmVersion(evmVersion),
		m_currentContractInvars(), m_currentSumSpecs(), m_builtinFunctions(),
		m_transferIncluded(false), m_callIncluded(false), m_sendIncluded(false),
		m_warnForBalances(false)
{
	// Initialize global declarations
	addGlobalComment("Global declarations and definitions");
	// address type
	addDecl(addressType());
	// address.balance
	m_boogieBalance = bg::Decl::variable(ASTBoogieUtils::BALANCE.boogie, bg::Decl::arraytype(addressType(), intType(256)));
	addDecl(m_boogieBalance);
	// This, sender, value
	m_boogieThis = bg::Decl::variable(ASTBoogieUtils::THIS.boogie, addressType());
	m_boogieMsgSender = bg::Decl::variable(ASTBoogieUtils::SENDER.boogie, addressType());
	m_boogieMsgValue = bg::Decl::variable(ASTBoogieUtils::VALUE.boogie, intType(256));
	// now
	addDecl(bg::Decl::variable(ASTBoogieUtils::NOW.boogie, intType(256)));
	// block number
	addDecl(bg::Decl::variable(ASTBoogieUtils::BLOCKNO.boogie, intType(256)));
	// overflow
	if (m_overflow)
		addDecl(bg::Decl::variable(ASTBoogieUtils::VERIFIER_OVERFLOW, boolType()));
	// Allocation counter
	m_boogieAllocCounter = bg::Decl::variable(ASTBoogieUtils::BOOGIE_ALLOC_COUNTER, bg::Decl::elementarytype("int"));
	addDecl(m_boogieAllocCounter);
}

bg::VarDeclRef BoogieContext::freshTempVar(bg::TypeDeclRef type, string prefix)
{
	return bg::Decl::variable(prefix + "#" + util::toString(nextId()), type);
}

void BoogieContext::printErrors(ostream& out)
{
	SourceReferenceFormatter formatter(out);
	for (auto const& error: errorReporter()->errors())
		formatter.printExceptionInformation(*error,
				(error->type() == Error::Type::Warning) ? "solc-verify warning" : "solc-verify error");
}

void BoogieContext::getPath(bg::Expr::Ref expr, SumPath& path, ASTNode const* errors)
{
	shared_ptr<bg::VarExpr const> id = nullptr;
	if (auto arrSelExpr = dynamic_pointer_cast<bg::ArrSelExpr const>(expr))
	{
		// Check if we reached a state var: x[this]
		if (auto bgThis = dynamic_pointer_cast<bg::VarExpr const>(arrSelExpr->getIdx()))
			if (bgThis->getName() == boogieThis()->getName())
				id = dynamic_pointer_cast<bg::VarExpr const>(arrSelExpr->getBase());


		if (!id)
		{
			// Check if we reached an indexer: x[this][i]
			auto subArrSelExpr = dynamic_pointer_cast<bg::ArrSelExpr const>(arrSelExpr->getBase());

			if (!subArrSelExpr)
			{
				// For arrays: skip inner array selection: x[this].innerarray[i]
				if (auto subMemAccExpr = dynamic_pointer_cast<bg::DtSelExpr const>(arrSelExpr->getBase()))
					subArrSelExpr = dynamic_pointer_cast<bg::ArrSelExpr const>(subMemAccExpr->getBase());
			}

			// Now get the id from the state var x[this]
			if (subArrSelExpr)
				if (auto bgThis = dynamic_pointer_cast<bg::VarExpr const>(subArrSelExpr->getIdx()))
					if (bgThis->getName() == boogieThis()->getName())
						id = dynamic_pointer_cast<bg::VarExpr const>(subArrSelExpr->getBase());

			if (!id && errors)
				reportError(errors, "Base of indexer must be identifier");
		}
	}

	if (id)
	{
		solAssert(path.base == "", "Base set twice");
		path.base = id->getName();
		return;
	}

	// Member access add member to path and recurse, e.g., x[this][i].y.z
	// adds 'z' to the path and recureses on x[this][i].y
	if (auto memAcc = dynamic_pointer_cast<bg::DtSelExpr const>(expr))
	{
		path.members.insert(path.members.begin(), memAcc);
		getPath(memAcc->getBase(), path, errors);
		return;
	}

	if (errors)
		reportError(errors, "Unsupported expression to sum over");
}

bool BoogieContext::pathsEqual(SumPath const& p1, SumPath const& p2)
{
	if (p1.base != p2.base)
		return false;
	if (p1.members.size() != p2.members.size())
		return false;
	for (unsigned i = 0; i < p1.members.size(); ++i)
		if (p1.members[i]->getMember() != p2.members[i]->getMember())
			return false;
	return true;
}

bg::Expr::Ref BoogieContext::getSumVar(bg::Expr::Ref bgExpr, Expression const* expr, TypePointer type)
{
	// Expression must be int, or an int array/mapping
	auto intType = dynamic_cast<IntegerType const*>(expr->annotation().type);
	if (auto arrType = dynamic_cast<ArrayType const*>(expr->annotation().type))
		intType = dynamic_cast<IntegerType const*>(arrType->baseType());
	if (auto mapType = dynamic_cast<MappingType const*>(expr->annotation().type))
		intType = dynamic_cast<IntegerType const*>(mapType->valueType());

	if (!intType)
	{
		reportError(expr, "Argument of sum must be an array, mapping or member access to integers");
		return bg::Expr::error();
	}

	SumPath path = {"", {}};
	getPath(bgExpr, path, expr);

	solAssert(path.base != "", "Could not get path to sum declaration");

	// Check if there is a spec already
	SumSpec* spec = nullptr;
	for (auto contr: m_currentContract->annotation().linearizedBaseContracts)
	{
		for (auto existingSpec: m_currentSumSpecs[contr])
		{
			if (pathsEqual(existingSpec.path, path))
			{
				spec = &existingSpec;
				break;
			}
		}
	}

	if (!spec) // If not, create new
	{
		string shadowName = path.base;
		for (auto mem: path.members)
			shadowName += "#" + mem->getMember();
		bg::VarDeclRef shadow = bg::Decl::variable(shadowName + "#SUM",
				bg::Decl::arraytype(addressType(), toBoogieType(type, expr)));
		addDecl(shadow);
		m_currentSumSpecs[m_currentContract].push_back({path, type, shadow});
		spec = &m_currentSumSpecs[m_currentContract].back();
	}

	solAssert(spec, "Sum specification not found nor created");
	return bg::Expr::arrsel(spec->shadowVar->getRefTo(), boogieThis()->getRefTo());
}

list<bg::Stmt::Ref> BoogieContext::initSumVars(Declaration const* decl)
{
	list<bg::Stmt::Ref> init;
	for (auto contr: m_currentContract->annotation().linearizedBaseContracts)
		for (auto spec: m_currentSumSpecs[contr])
			if (spec.path.base == mapDeclName(*decl))
				init.push_back(bg::Stmt::assign(
						bg::Expr::arrsel(spec.shadowVar->getRefTo(), boogieThis()->getRefTo()),
						intLit(0, 256)));
	return init;
}

list<bg::Stmt::Ref> BoogieContext::updateSumVars(bg::Expr::Ref lhsBg, bg::Expr::Ref rhsBg)
{
	SumPath path = {"", {}};
	getPath(lhsBg, path);

	if (path.base == "")
		return {}; // No need to sum

	// Otherwise go through all specs
	list<bg::Stmt::Ref> sumUpdates;
	for (auto contr: m_currentContract->annotation().linearizedBaseContracts)
	{
		for (auto spec: m_currentSumSpecs[contr])
		{
			if (spec.path.base == path.base && spec.path.members.size() >= path.members.size())
			{
				// Check for matching prefix, e.g., if sum is over ss[idx].t.z and
				// the lhs is ss[i].t then the matching prefix is until ss[idx].t
				bool match = true;
				unsigned i = 0;
				while (i < path.members.size())
				{
					if (spec.path.members[i]->getMember() != path.members[i]->getMember())
						match = false;
					++i;
				}
				if (!match)
					continue;

				// Remaining part of the path (.z) has to be added to both LHS and RHS
				while (i < spec.path.members.size())
				{
					lhsBg = spec.path.members[i]->replaceBase(lhsBg);
					rhsBg = spec.path.members[i]->replaceBase(rhsBg);
					++i;
				}

				unsigned bits = ASTBoogieUtils::getBits(spec.type);
				bool isSigned = ASTBoogieUtils::isSigned(spec.type);
				// arr[i] = x becomes arr#sum := arr#sum[this := ((arr#sum[this] - arr[i]) + x)]
				auto selExpr = bg::Expr::arrsel(spec.shadowVar->getRefTo(), boogieThis()->getRefTo());
				auto subResult = ASTBoogieUtils::encodeArithBinaryOp(*this, nullptr, Token::Sub, selExpr, lhsBg, bits, isSigned);
				auto addResult = ASTBoogieUtils::encodeArithBinaryOp(*this, nullptr, Token::Add, subResult.expr, rhsBg, bits, isSigned);
				if (encoding() == Encoding::MOD && !isSigned)
				{
					sumUpdates.push_back(bg::Stmt::comment("Implicit assumption that unsigned sum cannot underflow."));
					sumUpdates.push_back(bg::Stmt::assume(subResult.cc));
				}
				sumUpdates.push_back(bg::Stmt::assign(selExpr, addResult.expr));
			}
		}
	}
	return sumUpdates;
}

list<bg::Stmt::Ref> BoogieContext::havocSumVars()
{
	list<bg::Stmt::Ref> havocs;
	for (auto entry: m_currentSumSpecs)
		for (auto spec: entry.second)
				havocs.push_back(bg::Stmt::havoc(spec.shadowVar->getName()));
	return havocs;
}

string BoogieContext::mapDeclName(Declaration const& decl)
{
	std::string name = decl.name();

	// Check for special names
	if (dynamic_cast<MagicVariableDeclaration const*>(&decl))
	{
		// Special identifiers with the same name in Solidity and Boogie
		if (name == ASTBoogieUtils::SOLIDITY_ASSERT) return name;
		if (name == ASTBoogieUtils::SOLIDITY_REQUIRE) return name;
		if (name == ASTBoogieUtils::SOLIDITY_REVERT) return name;
		if (name == ASTBoogieUtils::SOLIDITY_KECCAK256) return name;
		if (name == ASTBoogieUtils::SOLIDITY_SHA256) return name;
		if (name == ASTBoogieUtils::SOLIDITY_RIPEMD160) return name;
		if (name == ASTBoogieUtils::SOLIDITY_ECRECOVER) return name;

		// Special identifiers with a different name in Solidity and Boogie
		if (name == ASTBoogieUtils::BALANCE.solidity) return ASTBoogieUtils::BALANCE.boogie;
		if (name == ASTBoogieUtils::TRANSFER.solidity) return ASTBoogieUtils::TRANSFER.boogie;
		if (name == ASTBoogieUtils::SEND.solidity) return ASTBoogieUtils::SEND.boogie;
		if (name == ASTBoogieUtils::CALL.solidity) return ASTBoogieUtils::CALL.boogie;
		if (name == ASTBoogieUtils::SUPER.solidity) return ASTBoogieUtils::SUPER.boogie;
		if (name == ASTBoogieUtils::THIS.solidity) return ASTBoogieUtils::THIS.boogie;
		if (name == ASTBoogieUtils::SENDER.solidity) return ASTBoogieUtils::SENDER.boogie;
		if (name == ASTBoogieUtils::VALUE.solidity) return ASTBoogieUtils::VALUE.boogie;
		if (name == ASTBoogieUtils::NOW.solidity) return ASTBoogieUtils::NOW.boogie;
		if (name == ASTBoogieUtils::BLOCKNO.solidity) return ASTBoogieUtils::BLOCKNO.boogie;
	}

	// ID is important to append, since (1) even fully qualified names can be
	// same for state variables and local variables in functions, (2) return
	// variables might have no name (whereas Boogie requires a name)
	name = name + "#" + to_string(decl.id());

	// Check if the current declaration is enclosed by any of the
	// extra scopes, if yes, add extra ID
	for (auto extraScope: m_extraScopes)
	{
		ASTNode const* running = decl.scope();
		while (running)
		{
			if (running == extraScope.first)
			{
				name += "#" + extraScope.second;
				break;
			}
			running = scopes()[running]->enclosingNode();
		}
	}

	return name;
}

void BoogieContext::warnForBalances()
{
	if (!m_warnForBalances)
	{
		m_errorReporter->warning(5042_error, "Balance modifications due to gas consumption or miner rewards are not modeled");
	}
	m_warnForBalances = true;
}

bg::Expr::Ref BoogieContext::getStringLiteral(string literal)
{
	return ASTBoogieUtils::stringValue(*this, literal);
}

bg::Expr::Ref BoogieContext::getAddressLiteral(string addr)
{
	if (m_addressLiterals.find(addr) == m_addressLiterals.end())
	{
		auto addrConst = bg::Decl::constant("address_" + addr, addressType(), true);
		m_addressLiterals[addr] = addrConst;
		addDecl(addrConst);
	}
	return m_addressLiterals[addr]->getRefTo();
}

void BoogieContext::addBuiltinFunction(bg::FuncDeclRef fnDecl)
{
	m_builtinFunctions[fnDecl->getName()] = fnDecl;
	m_program.getDeclarations().push_back(fnDecl);
}

void BoogieContext::includeTransferFunction()
{
	if (m_transferIncluded) return;
	m_transferIncluded = true;
	m_program.getDeclarations().push_back(ASTBoogieUtils::createTransferProc(*this));
}

void BoogieContext::includeCallFunction()
{
	if (m_callIncluded) return;
	m_callIncluded = true;
	m_program.getDeclarations().push_back(ASTBoogieUtils::createCallProc(*this));
}

void BoogieContext::includeSendFunction()
{
	if (m_sendIncluded) return;
	m_sendIncluded = true;
	m_program.getDeclarations().push_back(ASTBoogieUtils::createSendProc(*this));
}

void BoogieContext::reportError(ASTNode const* associatedNode, string message)
{
	solAssert(associatedNode, "Error at unknown node: " + message);
	m_errorReporter->error(2114_error, Error::Type::ParserError, associatedNode->location(), message);
}

void BoogieContext::reportWarning(ASTNode const* associatedNode, string message)
{
	solAssert(associatedNode, "Warning at unknown node: " + message);
	m_errorReporter->warning(3200_error, associatedNode->location(), message);
}

void BoogieContext::addGlobalComment(string str)
{
	addDecl(bg::Decl::comment("", str));
}

void BoogieContext::addDecl(bg::Decl::Ref decl)
{
	m_program.getDeclarations().push_back(decl);
}

bg::TypeDeclRef BoogieContext::addressType() const
{
	bg::TypeDeclRef it = intType(256);
	return bg::Decl::aliasedtype("address_t", it);
}

bg::TypeDeclRef BoogieContext::boolType() const
{
	return bg::Decl::elementarytype("bool");
}

bg::TypeDeclRef BoogieContext::intType(unsigned size) const
{
	if (isBvEncoding())
		return bg::Decl::elementarytype("bv" + util::toString(size));
	else
		return bg::Decl::elementarytype("int");
}

bg::TypeDeclRef BoogieContext::localPtrType()
{
	return bg::Decl::arraytype(intType(256), intType(256));
}

bg::FuncDeclRef BoogieContext::getStructConstructor(StructDefinition const* structDef)
{
	if (m_storStructConstrs.find(structDef) == m_storStructConstrs.end())
	{
		vector<bg::Binding> params;

		for (auto member: structDef->members())
		{
			// Make sure that the location of the member is storage (this is
			// important for struct members as there is a single type per struct
			// definition, which is storage pointer by default).
			// TODO: can we do better?
			TypePointer memberType = TypeProvider::withLocationIfReference(DataLocation::Storage, member->type());
			params.push_back({
				bg::Expr::id(mapDeclName(*member)),
				toBoogieType(memberType, structDef)});
		}

		vector<bg::Attr::Ref> attrs;
		attrs.push_back(bg::Attr::attr("constructor"));
		string name = structDef->name() + "#" + util::toString(structDef->id()) + "#constr";
		m_storStructConstrs[structDef] = bg::Decl::function(name, params,
				getStructType(structDef, DataLocation::Storage), nullptr, attrs);
		addDecl(m_storStructConstrs[structDef]);
	}
	return m_storStructConstrs[structDef];
}

bg::TypeDeclRef BoogieContext::getStructType(StructDefinition const* structDef, DataLocation loc)
{
	string typeName = "struct_" + ASTBoogieUtils::dataLocToStr(
			loc == DataLocation::CallData ? DataLocation::Memory : loc) +
			"_" + structDef->name() + "#" + util::toString(structDef->id());

	if (loc == DataLocation::Storage)
	{
		if (m_storStructTypes.find(structDef) == m_storStructTypes.end())
		{
			vector<bg::Binding> members;
			for (auto member: structDef->members())
			{
				// Make sure that the location of the member is storage (this is
				// important for struct members as there is a single type per struct
				// definition, which is storage pointer by default).
				// TODO: can we do better?
				TypePointer memberType = TypeProvider::withLocationIfReference(loc, member->type());
				members.push_back({bg::Expr::id(mapDeclName(*member)),
					toBoogieType(memberType, structDef)});
			}
			m_storStructTypes[structDef] = bg::Decl::datatype(typeName, members);
			addDecl(m_storStructTypes[structDef]);
		}
		return m_storStructTypes[structDef];
	}
	else if (loc == DataLocation::Memory || loc == DataLocation::CallData)
	{
		if (m_memStructTypes.find(structDef) == m_memStructTypes.end())
		{
			m_memStructTypes[structDef] = bg::Decl::aliasedtype("address_" + typeName, bg::Decl::elementarytype("int"));
			addDecl(m_memStructTypes[structDef]);
		}
		return m_memStructTypes[structDef];
	}
	reportError(structDef, "Unsupported data location (" + ASTBoogieUtils::dataLocToStr(loc) + ") for struct.");
	return errType();
}

bg::VarDeclRef BoogieContext::getDefaultStorageContext(StructType const* type)
{
	auto structDef = &type->structDefinition();
	if (m_defaultStorageContexts.find(structDef) == m_defaultStorageContexts.end())
	{
		auto varDecl = bg::Decl::variable(
				structDef->name() + util::toString(structDef->id()) + "default_context",
				bg::Decl::arraytype(intType(256), getStructType(structDef, DataLocation::Storage)));
		m_defaultStorageContexts[structDef] = varDecl;
		addDecl(varDecl);
	}
	return m_defaultStorageContexts[structDef];
}

bg::VarDeclRef BoogieContext::getDefaultStorageContext(ArrayType const* type)
{
	auto baseBgType = toBoogieType(type->baseType(), nullptr);
	string baseBgTypeStr = cleanupTypeName(baseBgType->getName());
	if (m_defaultArrStorageContexts.find(baseBgTypeStr) == m_defaultArrStorageContexts.end())
	{
		auto varDecl = bg::Decl::variable(
				baseBgTypeStr + "_arr_default_context",
				bg::Decl::arraytype(intType(256), getArrayDatatype(baseBgType)));
		m_defaultArrStorageContexts[baseBgTypeStr] = varDecl;
		addDecl(varDecl);
	}
	return m_defaultArrStorageContexts[baseBgTypeStr];
}

std::string BoogieContext::cleanupTypeName(std::string typeName)
{
	boost::replace_all(typeName, "[", "_k_");
	boost::replace_all(typeName, "]", "_v_");
	return typeName;
}

void BoogieContext::ensureArrayDeclarations(bg::TypeDeclRef valueType)
{
	auto valueTypeName = valueType->getName();
	if (m_arrConstrs.find(valueTypeName) != m_arrConstrs.end())
		return;
	solAssert(m_arrDataTypes.find(valueTypeName) == m_arrDataTypes.end(), "Should have been declared in parallel");

	// Have to declare a new one, clean up the name
	string valueTypeNameNoSpec = cleanupTypeName(valueTypeName);

	// Datatype: [int]T + length
	vector<bg::Binding> members = {
			{ bg::Expr::id("arr"), bg::Decl::arraytype(intType(256), valueType) },
			{ bg::Expr::id("length"), intType(256) }
	};
	bg::DataTypeDeclRef datatypeDecl =  bg::Decl::datatype(valueTypeNameNoSpec + "_arr_type", members);
	m_arrDataTypes[valueTypeName] = datatypeDecl;
	addDecl(datatypeDecl);

	// Constructor for
	string name = valueTypeNameNoSpec + "_arr#constr";
	bg::FuncDeclRef arrayConstructor = bg::Decl::function(name, members, datatypeDecl, nullptr,
			{ bg::Attr::attr("constructor") });
	m_arrConstrs[valueTypeName] = arrayConstructor;
	addDecl(arrayConstructor);
}

bg::FuncDeclRef BoogieContext::getArrayConstructor(bg::TypeDeclRef valueType)
{
	ensureArrayDeclarations(valueType);
	return m_arrConstrs[valueType->getName()];
}

bg::DataTypeDeclRef BoogieContext::getArrayDatatype(bg::TypeDeclRef valueType)
{
	ensureArrayDeclarations(valueType);
	return m_arrDataTypes[valueType->getName()];
}

bg::FuncDeclRef BoogieContext::defaultArray(bg::TypeDeclRef keyType, bg::TypeDeclRef valueType, string valueSmt)
{
	string funcName = "default_" + keyType->getName() + "_" + valueType->getName();
	boost::replace_all(funcName, "[", "_k_");
	boost::replace_all(funcName, "]", "_v_");

	if (m_defaultArrays.find(funcName) == m_defaultArrays.end())
	{
		vector<bg::Attr::Ref> attrs;
		attrs.push_back(bg::Attr::attr("builtin", valueSmt));
		auto funcDecl = bg::Decl::function(funcName, {}, bg::Decl::arraytype(keyType, valueType), nullptr, attrs);
		addBuiltinFunction(funcDecl);
		m_defaultArrays[funcName] = funcDecl;
	}
	return m_defaultArrays[funcName];
}

bg::Stmt::Ref BoogieContext::incrAllocCounter()
{
	return bg::Stmt::assign(
			m_boogieAllocCounter->getRefTo(),
			bg::Expr::plus(m_boogieAllocCounter->getRefTo(), bg::Expr::intlit((long)1)));
}

bg::Expr::Ref BoogieContext::getMemArray(bg::Expr::Ref arrPtrExpr, bg::TypeDeclRef type)
{
	return bg::Expr::arrsel(m_memArrs[type->getName()]->getRefTo(), arrPtrExpr);
}

bg::Expr::Ref BoogieContext::getArrayLength(bg::Expr::Ref arrayExpr, bg::TypeDeclRef type)
{
	auto arrayConstructor = getArrayConstructor(type);
	auto arrayDataType = getArrayDatatype(type);
	return bg::Expr::dtsel(arrayExpr, "length", arrayConstructor, arrayDataType);
}

bg::Expr::Ref BoogieContext::getInnerArray(bg::Expr::Ref arrayExpr, bg::TypeDeclRef type)
{
	auto arrayConstructor = getArrayConstructor(type);
	auto arrayDataType = getArrayDatatype(type);
	return bg::Expr::dtsel(arrayExpr, "arr", arrayConstructor, arrayDataType);
}

bg::TypeDeclRef BoogieContext::toBoogieType(TypePointer tp, ASTNode const* _associatedNode)
{
	Type::Category tpCategory = tp->category();

	switch (tpCategory)
	{
	case Type::Category::Address:
		return addressType();
	case Type::Category::StringLiteral:
		return toBoogieType(TypeProvider::stringStorage(), _associatedNode);
	case Type::Category::Bool:
		return boolType();
	case Type::Category::RationalNumber:
	{
		auto tpRational = dynamic_cast<RationalNumberType const*>(tp);
		if (!tpRational->isFractional())
			return toBoogieType(tpRational->integerType(), _associatedNode);
		else
			reportError(_associatedNode, "Fractional numbers are not supported");
		break;
	}
	case Type::Category::Integer:
	{
		auto tpInteger = dynamic_cast<IntegerType const*>(tp);
		return intType(tpInteger->numBits());
	}
	case Type::Category::Contract:
		return addressType();
	case Type::Category::Array:
	{
		auto arrType = dynamic_cast<ArrayType const*>(tp);
		auto baseType = arrType->baseType();
		auto baseTypeDecl = toBoogieType(baseType, _associatedNode);
		string baseName = baseTypeDecl->getName();

		if (arrType->location() == DataLocation::Storage && arrType->isPointer())
			return localPtrType();
		// Storage arrays are simply the data structures
		if (arrType->location() == DataLocation::Storage)
			return getArrayDatatype(baseTypeDecl);
		// Memory arrays have an extra layer of indirection
		// TODO: for precision, calldata arrays could be translated to different
		// mappings than memory
		else if (arrType->location() == DataLocation::Memory || arrType->location() == DataLocation::CallData)
		{
			if (m_memArrPtrTypes.find(baseName) == m_memArrPtrTypes.end())
			{
				// Pointer type
				std::string baseNameNoSpec = cleanupTypeName(baseName);
				m_memArrPtrTypes[baseName] = bg::Decl::aliasedtype(baseNameNoSpec + "_arr_ptr", bg::Decl::elementarytype("int"));
				addDecl(m_memArrPtrTypes[baseName]);

				// The actual storage
				auto arrayDataType = getArrayDatatype(baseTypeDecl);
				m_memArrs[baseName] = bg::Decl::variable("mem_arr_" + baseNameNoSpec,
						bg::Decl::arraytype(m_memArrPtrTypes[baseName], arrayDataType));
				addDecl(m_memArrs[baseName]);
			}
			return m_memArrPtrTypes[baseName];
		}
		else
		{
			reportError(_associatedNode, "Unsupported location (" +
					ASTBoogieUtils::dataLocToStr(arrType->location()) + ") for array type");
			return errType();
		}
		break;
	}
	case Type::Category::Mapping:
	{
		auto mapType = dynamic_cast<MappingType const*>(tp);
		auto keyType = mapType->keyType();
		if (tp->dataStoredIn(DataLocation::Storage))
			keyType = TypeProvider::withLocationIfReference(DataLocation::Storage, keyType);
		auto valueType = mapType->valueType();
		return bg::Decl::arraytype(toBoogieType(keyType, _associatedNode),
				toBoogieType(valueType, _associatedNode));
	}
	case Type::Category::FixedBytes:
	{
		// up to 32 bytes (use integer and slice it up)
		auto fbType = dynamic_cast<FixedBytesType const*>(tp);
		return intType(fbType->numBytes() * 8);
	}
	case Type::Category::Tuple:
		reportError(_associatedNode, "Tuples are not supported");
		break;
	case Type::Category::Struct:
	{
		auto structTp = dynamic_cast<StructType const*>(tp);
		// Local pointers are arrays
		if (structTp->location() == DataLocation::Storage && structTp->isPointer())
			return localPtrType();

		return getStructType(&structTp->structDefinition(), structTp->location());
	}
	case Type::Category::Enum:
		return intType(256);
	default:
		std::string tpStr = tp->toString();
		reportError(_associatedNode, "Unsupported type: '" + tpStr.substr(0, tpStr.find(' ')) + "'");
	}

	return errType();
}

bg::Expr::Ref BoogieContext::intLit(bg::bigint lit, unsigned bits) const
{
	if (isBvEncoding())
		return bg::Expr::bvlit(lit, bits);
	else
		return bg::Expr::intlit(lit);
}

bg::Expr::Ref BoogieContext::intSlice(bg::Expr::Ref base, unsigned size, unsigned high, unsigned low)
{
	solAssert(high < size, "");
	solAssert(low < high, "");
	if (isBvEncoding())
		return bvExtract(base, size, high, low);
	else
	{
		bg::Expr::Ref result = base;
		if (low > 0)
		{
			bg::Expr::Ref c1 = bg::Expr::intlit(bg::bigint(2) << (low - 1));
			result = bg::Expr::intdiv(result, c1);
		}
		if (high < size - 1)
		{
			bg::Expr::Ref c2 = bg::Expr::intlit(bg::bigint(2) << (high - low));
			result = bg::Expr::mod(result, c2);
		}
		return result;
	}
}

bg::Expr::Ref BoogieContext::bvExtract(bg::Expr::Ref expr, unsigned exprSize, unsigned high, unsigned low)
{
	// Function name
	std::stringstream fnNameSS;
	fnNameSS << "extract_" << high << "_to_" << low << "_from_" << exprSize;
	std::string fnName = fnNameSS.str();

	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Not there construct SMT
		std::stringstream fnSmtSS;
		fnSmtSS << "(_ extract " << high << " " << low << "0)";

		// Appropriate types
		unsigned resultSize = high - low + 1;
		bg::TypeDeclRef resultType = intType(resultSize);
		bg::TypeDeclRef exprType = intType(exprSize);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
				fnNameSS.str(),     // Name
				{ { bg::Expr::id(""), exprType} }, // Arguments
				resultType,         // Type
				nullptr,            // Body = null
				{ bg::Attr::attr("bvbuiltin", fnSmtSS.str()) } // Attributes
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, expr);
}

bg::Expr::Ref BoogieContext::bvZeroExt(bg::Expr::Ref expr, unsigned exprSize, unsigned resultSize)
{
	// Function name
	std::stringstream fnNameSS;
	fnNameSS << "bvzeroext_" << exprSize << "_to_" << resultSize;
	std::string fnName = fnNameSS.str();

	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Not there construct SMT
		std::stringstream fnSmtSS;
		fnSmtSS << "(_ zero_extend " << resultSize - exprSize << ")";

		// Appropriate types
		bg::TypeDeclRef resultType = intType(resultSize);
		bg::TypeDeclRef exprType = intType(exprSize);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
				fnNameSS.str(),     // Name
				{ { bg::Expr::id(""), exprType} }, // Arguments
				resultType,         // Type
				nullptr,            // Body = null
				{ bg::Attr::attr("bvbuiltin", fnSmtSS.str()) } // Attributes
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, expr);
}

bg::Expr::Ref BoogieContext::bvSignExt(bg::Expr::Ref expr, unsigned exprSize, unsigned resultSize)
{
	// Function name
	std::stringstream fnNameSS;
	fnNameSS << "bvsignext_" << exprSize << "_to_" << resultSize;
	std::string fnName = fnNameSS.str();

	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Not there construct SMT
		std::stringstream fnSmtSS;
		fnSmtSS << "(_ sign_extend " << resultSize - exprSize << ")";

		// Appropriate types
		bg::TypeDeclRef resultType = intType(resultSize);
		bg::TypeDeclRef exprType = intType(exprSize);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
				fnNameSS.str(),     // Name
				{ { bg::Expr::id(""), exprType} }, // Arguments
				resultType,         // Type
				nullptr,            // Body = null
				{ bg::Attr::attr("bvbuiltin", fnSmtSS.str()) } // Attributes
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, expr);
}

bg::Expr::Ref BoogieContext::bvNeg(unsigned bits, bg::Expr::Ref expr)
{
	return bvUnaryOp("neg", bits, expr);
}

bg::Expr::Ref BoogieContext::bvNot(unsigned bits, bg::Expr::Ref expr)
{
	return bvUnaryOp("not", bits, expr);
}


bg::Expr::Ref BoogieContext::bvAdd(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("add", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvSub(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("sub", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvMul(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("mul", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvSDiv(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("sdiv", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvUDiv(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("udiv", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvAnd(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("and", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvOr(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("or", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvXor(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("xor", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvAShr(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("ashr", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvLShr(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("lshr", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvShl(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("shl", bits, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvSlt(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("slt", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvUlt(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("ult", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvSgt(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("sgt", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvUgt(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("ugt", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvSle(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("sle", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvUle(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("ule", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvSge(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("sge", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvUge(unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs)
{
	return bvBinaryOp("uge", bits, lhs, rhs, boolType());
}

bg::Expr::Ref BoogieContext::bvBinaryOp(std::string name, unsigned bits, bg::Expr::Ref lhs, bg::Expr::Ref rhs, bg::TypeDeclRef resultType)
{
	// Function name
	std::stringstream fnNameSS;
	fnNameSS << "bv" << bits << name;
	std::string fnName = fnNameSS.str();

	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Not there construct SMT
		std::stringstream fnSmtSS;
		fnSmtSS << "bv" << name;

		// Appropriate types
		if (resultType == nullptr)
			resultType = intType(bits);
		bg::TypeDeclRef exprType = intType(bits);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
				fnNameSS.str(),     // Name
				{ { bg::Expr::id(""), exprType }, { bg::Expr::id(""), exprType } }, // Arguments
				resultType,         // Type
				nullptr,            // Body = null
				{ bg::Attr::attr("bvbuiltin", fnSmtSS.str()) } // Attributes
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, lhs, rhs);
}

bg::Expr::Ref BoogieContext::bvUnaryOp(std::string name, unsigned bits, bg::Expr::Ref expr)
{
	// Function name
	std::stringstream fnNameSS;
	fnNameSS << "bv" << bits << name;
	std::string fnName = fnNameSS.str();

	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Not there construct SMT
		std::stringstream fnSmtSS;
		fnSmtSS << "bv" << name;

		// Appropriate types
		bg::TypeDeclRef exprType = intType(bits);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
				fnNameSS.str(),       // Name
				{ { bg::Expr::id(""), exprType } }, // Arguments
				exprType,  // Type
				nullptr,              // Body = null
				{ bg::Attr::attr("bvbuiltin", fnSmtSS.str()) } // Attributes
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, expr);
}


bg::Expr::Ref BoogieContext::hashFunction(std::string fnName, FunctionType::Kind fnKind, bg::Expr::Ref arg)
{
	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Appropriate types
		FunctionType const* fnType = nullptr;
		GlobalContext globals;
		auto declarations = globals.declarations();
		for (auto const& decl: declarations)
		{
			auto declType = decl->type();
			fnType = dynamic_cast<FunctionType const*>(declType);
			if (fnType)
			{
				FunctionType::Kind kind = fnType->kind();
				if (kind == fnKind)
					break;
			}
		}

		bg::TypeDeclRef argType = toBoogieType(fnType->parameterTypes().front(), nullptr);
		bg::TypeDeclRef returnType = toBoogieType(fnType->returnParameterTypes().front(), nullptr);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
			fnName, // Name
			{ { bg::Expr::id(""), argType } }, // Arguments
			returnType, // Return type
			nullptr // Body = null
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, arg);
}

bg::Expr::Ref BoogieContext::keccak256(bg::Expr::Ref arg)
{
	return hashFunction(ASTBoogieUtils::SOLIDITY_KECCAK256, FunctionType::Kind::KECCAK256, arg);
}

bg::Expr::Ref BoogieContext::sha256(bg::Expr::Ref arg)
{
	return hashFunction(ASTBoogieUtils::SOLIDITY_SHA256, FunctionType::Kind::SHA256, arg);
}

bg::Expr::Ref BoogieContext::ripemd160(bg::Expr::Ref arg)
{
	return hashFunction(ASTBoogieUtils::SOLIDITY_RIPEMD160, FunctionType::Kind::RIPEMD160, arg);
}

bg::Expr::Ref BoogieContext::ecrecover(bg::Expr::Ref hash, bg::Expr::Ref v, bg::Expr::Ref r, bg::Expr::Ref s)
{
	std::string fnName = ASTBoogieUtils::SOLIDITY_ECRECOVER;

	// Get it if already there
	if (m_builtinFunctions.find(fnName) == m_builtinFunctions.end())
	{
		// Appropriate types
		FunctionType const* fnType = nullptr;
		GlobalContext globals;
		auto declarations = globals.declarations();
		for (auto const& decl: declarations)
		{
			auto declType = decl->type();
			fnType = dynamic_cast<FunctionType const*>(declType);
			if (fnType)
			{
				FunctionType::Kind kind = fnType->kind();
				if (kind == FunctionType::Kind::ECRecover)
					break;
			}
		}

		std::vector<bg::Binding> argTypes;
		for (TypePointer argType: fnType->parameterTypes())
		{
			bg::TypeDeclRef argTypeBg = toBoogieType(argType, nullptr);
			argTypes.push_back({ bg::Expr::id(""), argTypeBg});
		}

		bg::TypeDeclRef returnType = toBoogieType(fnType->returnParameterTypes().front(), nullptr);

		// Boogie declaration
		bg::FuncDeclRef fnDecl = bg::Decl::function(
			fnName, // Name
			argTypes, // Arguments
			returnType, // Return type
			nullptr // Body = null
		);

		// Add it
		addBuiltinFunction(fnDecl);
	}

	return bg::Expr::fn(fnName, {hash, v, r, s});
}

bg::Expr::Subst const& BoogieContext::getEventDataSubstitution() const
{
	return m_eventDataSubstitution;
}

void BoogieContext::addEventData(Expression const* expr, EventDefinition const* event)
{
	// Get the variable
	auto dataExpr = dynamic_cast<Identifier const*>(expr);
	solAssert(dataExpr, "We only accept members");
	std::string dataVarName = mapDeclName(*dataExpr->annotation().referencedDeclaration);
	auto dataDecl = dataExpr->annotation().referencedDeclaration;

	// If expression already there, we can skip
	if (m_eventData[event].count(dataDecl) > 0)
		return;

	// Create the new variables and declare them
	if (m_allEventData.count(dataDecl) == 0)
	{
		string oldDataName = dataVarName + "#event_old";
		TypePointer type = dataExpr->annotation().type;
		bg::TypeDeclRef oldDataType = bg::Decl::arraytype(addressType(), toBoogieType(type, expr));
		bg::VarDeclRef oldDataDecl = bg::Decl::variable(oldDataName, oldDataType);
		addDecl(oldDataDecl);
		string oldUpdateName = dataVarName + "#event_old#saved";
		bg::TypeDeclRef oldUpdateType = bg::Decl::elementarytype("bool");
		bg::VarDeclRef oldUpdateDecl = bg::Decl::variable(oldUpdateName, oldUpdateType);
		addDecl(oldUpdateDecl);
		bg::Expr::Ref data = bg::Expr::id(dataVarName);
		bg::Expr::Ref oldData = bg::Expr::id(oldDataName);
		bg::Expr::Ref oldDataSaved = bg::Expr::id(oldUpdateName);
		m_allEventData[dataDecl].dataVar = data;
		m_allEventData[dataDecl].oldDataVar = oldData;
		m_allEventData[dataDecl].oldDataSavedVar = oldDataSaved;
		m_allEventData[dataDecl].events.insert(event->name());
		m_eventDataSubstitution[dataVarName] = bg::Expr::cond(oldDataSaved, oldData, data);
	}
	else
		m_allEventData[dataDecl].events.insert(event->name());

	// Record the data and the substitution
	m_eventData[event].insert(dataDecl);
}

void BoogieContext::enableEventDataTrackingFor(EventDefinition const* event)
{
	// If the
	solAssert(m_eventData.count(event), "Events need to be processed before enabling them");
	m_eventDataCurrent.insert(event);
}

void BoogieContext::disableEventDataTracking()
{
	m_eventDataCurrent.clear();
}

/** Helper class to compute all base expressions in LValue */
class ExprBaseComputation : private ASTConstVisitor
{
private:

	std::set<Identifier const*>& m_baseExpressions;

public:

	/**
	 * Create a new instance with a given context.
	 */
	ExprBaseComputation(std::set<Identifier const*>& modified)
	: m_baseExpressions(modified) {}

	/**	Get the base field variable of a lvale. */
	void run(ASTNode const& _node)
	{
		_node.accept(*this);
	}

	// Only need to handle expressions that have a base or where we
	// should report errors because unsupported
	bool visit(Conditional const& _node) override
	{
		_node.trueExpression().accept(*this);
		_node.falseExpression().accept(*this);
		return false;
	}

	bool visit(Assignment const& _node) override
	{
		_node.leftHandSide().accept(*this);
		return false;
	}

	bool visit(FunctionCall const& _node) override
	{
		_node.expression().accept(*this); // push/pop stuff
		return false;
	}

	bool visit(NewExpression const& _node) override
	{
		(void)_node;
		// new expressions can create assignments, we ignore
		return false;
	}

	bool visit(MemberAccess const& _node) override
	{
		_node.expression().accept(*this);
		return false;
	}

	bool visit(IndexAccess const& _node) override
	{
		_node.baseExpression().accept(*this);
		return false;
	}

	bool visit(Identifier const& _node) override
	{
		m_baseExpressions.insert(&_node);
		return false;
	}

	bool visit(TupleExpression const& _node) override
	{
		for (auto c: _node.components())
			if (c)
				c->accept(*this);
		return false;
	}

	bool visit(VariableDeclarationStatement const& _node) override
	{
		(void)_node;
		return false; // No need to process local variable declarations
	}

	bool visit(Return const& _node) override
	{
		(void)_node;
		return false; // No need to process return value assignments
	}

};

std::list<bg::Stmt::Ref> BoogieContext::checkForEventDataSave(ASTNode const* lhsExpr)
{
	// Get the base expressions and their declaration
	std::set<Identifier const*> baseExpressions;
	ExprBaseComputation computeBase(baseExpressions);
	computeBase.run(*lhsExpr);
	// TODO: go through the found identifiers and see which ones are storage pointers
	// those need to have special treatment
	std::set<Declaration const*> baseDeclarations;
	std::set<Declaration const*> updatedExpressions;
	for (auto const& id: baseExpressions)
		baseDeclarations.insert(id->annotation().referencedDeclaration);

	// Collect all events where the base expression is watched to see if it needs saving
	if (m_eventDataCurrent.size() > 0)
	{
		for (auto event: m_eventDataCurrent)
		{
			auto const& eventInfo = m_eventData[event];
			for (auto expr: eventInfo)
			{
				if (baseDeclarations.count(expr))
				{
					updatedExpressions.insert(expr);
					break;
				}
			}
		}
	}
	else
	{
		// Check that there is no event tracking this data
		std::set<string> errors;
		for (auto decl: baseDeclarations)
		{
			auto find = m_allEventData.find(decl);
			if (find != m_allEventData.end())
			{
				std::stringstream msg;
				msg << "'" << decl->name() << "' is tracked by";
				auto const& events = find->second.events;
				bool plural = events.size() > 1;
				msg << (plural ? " events" : " event");
				bool first = true;
				for (auto e: events)
				{
					msg << (first ? " '" : ", '") << e << "'";
					first = false;
				}
				msg << " but " << (plural ? "none are" : "it is not") << " specified to emit";
				errors.insert(msg.str());
			}
		}
		for (auto error: errors)
			reportError(lhsExpr, error);
	}

	// Make the statement if (!data_saved) { old_data = data; data_saved = true; }
	std::list<bg::Stmt::Ref> stmts;
	for (auto e: updatedExpressions)
	{
		auto const& info = m_allEventData[e];
		std::string dataVarName = mapDeclName(*e);
		bg::Expr::Ref dataVar = bg::Expr::id(dataVarName);
		bg::Block::Ref update = bg::Block::block();
		update->addStmts({
			bg::Stmt::assign(info.oldDataVar, dataVar),
			bg::Stmt::assign(info.oldDataSavedVar, bg::Expr::true_())
		});
		stmts.push_back(bg::Stmt::ifelse(bg::Expr::not_(info.oldDataSavedVar), update));
	}

	return stmts;
}

bg::ProcDeclRef BoogieContext::declareEventProcedure(EventDefinition const* event, std::string eventName, std::vector<bg::Binding> const& params, bool onlyOnChanges)
{
	// Event body sets up the variables
	vector<bg::Block::Ref> blocks;
	blocks.push_back(bg::Block::block());

	// Get event data and add the statements
	auto const& eventData = m_eventData[event];
	for (auto e: eventData)
	{
		bg::Expr::Ref dataSavedVar = m_allEventData[e].oldDataSavedVar;
		blocks.back()->addStmt(bg::Stmt::assign(dataSavedVar, bg::Expr::false_()));
	}

	// Declare postconditions
	auto procDecl = bg::Decl::procedure(eventName, params, {}, {}, blocks);

	// At least one data entry has been saved on entry, none has been saved on exit
	if (eventData.size())
	{
		std::vector<bg::Expr::Ref> dataSavedDisjuncts;
		for (auto const& e: eventData)
			dataSavedDisjuncts.push_back(m_allEventData[e].oldDataSavedVar);
		bg::Expr::Ref dataSaved = bg::Expr::or_(dataSavedDisjuncts);

		if (onlyOnChanges)
		{
			procDecl->getRequires().push_back(bg::Specification::spec(dataSaved,
				ASTBoogieUtils::createAttrs(event->location(), "Event triggered without changes to data", *currentScanner())));
		}
		procDecl->getEnsures().push_back(bg::Specification::spec(bg::Expr::not_(dataSaved),
			ASTBoogieUtils::createAttrs(event->location(), "Event triggered without changes to data", *currentScanner())));
	}

	procDecl->addAttr(bg::Attr::attr("inline", 1));

	return procDecl;
}

void BoogieContext::addFunctionSpecsForEvent(EventDefinition const* event, bg::ProcDeclRef procDecl)
{
	auto eventData = m_eventData[event];
	if (eventData.size())
	{
		std::vector<bg::Expr::Ref> dataSavedDisjuncts;
		for (auto e: eventData)
			dataSavedDisjuncts.push_back(m_allEventData[e].oldDataSavedVar);
		bg::Expr::Ref dataNotSaved = bg::Expr::not_(bg::Expr::or_(dataSavedDisjuncts));

		procDecl->getRequires().push_back(bg::Specification::spec(dataNotSaved,
				ASTBoogieUtils::createAttrs(event->location(), "Function called without triggering event " + event->name(), *currentScanner())));
		procDecl->getEnsures().push_back(bg::Specification::spec(dataNotSaved,
				ASTBoogieUtils::createAttrs(event->location(), "Function can end without triggering event", *currentScanner())));
	}
}

std::pair<boogie::Expr::Ref, std::string> BoogieContext::getEventLoopInvariant(EventDefinition const* event) const
{
	std::pair<boogie::Expr::Ref, std::string> result;
	auto eventData = m_eventData.find(event);
	if (eventData != m_eventData.end() && eventData->second.size())
	{
		std::vector<bg::Expr::Ref> dataSavedDisjuncts;
		for (auto e: eventData->second)
		{
			auto dataInfo = m_allEventData.find(e);
			solAssert(dataInfo != m_allEventData.end(), "Should have been added");
			dataSavedDisjuncts.push_back(dataInfo->second.oldDataSavedVar);
		}
		bg::Expr::Ref dataNotSaved =  bg::Expr::not_(bg::Expr::or_(dataSavedDisjuncts));
		result.first = dataNotSaved,
		result.second = "event " + event->name() + " must be triggered if event tracked data was modified";
	}
	return result;
}

void BoogieContext::setCurrentContract(ContractDefinition const* contract)
{
	if (contract)
	{
		m_globalContext.setCurrentContract(*contract);
		m_scopes[nullptr]->registerDeclaration(*m_globalContext.currentSuper(), nullptr, false, true);
		m_scopes[nullptr]->registerDeclaration(*m_globalContext.currentThis(), nullptr, false, true);
	}
	else
	{
		// make "this" and "super" invisible.
		m_scopes[nullptr]->registerDeclaration(*m_globalContext.currentThis(), nullptr, true, true);
		m_scopes[nullptr]->registerDeclaration(*m_globalContext.currentSuper(), nullptr, true, true);
		m_globalContext.resetCurrentContract();
	}
	m_currentContract = contract;
}


void ExprConditionStore::addCondition(std::string id, ConditionType type, boogie::Expr::Ref ref)
{
	m_conditionsOnVariables[id][type].insert(ref);
}

void ExprConditionStore::addCondition(ConditionType type, boogie::Expr::Ref ref)
{
	m_conditions[type].insert(ref);
}

void ExprConditionStore::addConditions(ExprConditionStore const& other)
{
	m_conditions.insert(other.m_conditions.begin(), other.m_conditions.end());
	m_conditionsOnVariables.insert(other.m_conditionsOnVariables.begin(), other.m_conditionsOnVariables.end());
}

ExprConditionStore::ConditionSet ExprConditionStore::getConditions(ExprConditionStore::ConditionType type) const
{
	ConditionSet result;

	auto find = m_conditions.find(type);
	if (find != m_conditions.end())
	{
		for (auto const& c: find->second)
			result.insert(c);
	}

	for (auto const& pair: m_conditionsOnVariables)
	{
		auto find = pair.second.find(type);
		if (find != pair.second.end())
			for (auto const& c: find->second)
				result.insert(c);
	}

	return result;
}

ExprConditionStore::ConditionSet ExprConditionStore::getConditionsContaining(ConditionType type, std::string id) const
{
	ConditionSet result;

	auto find = m_conditions.find(type);
	if (find != m_conditions.end())
	{
		for (auto const& c: find->second)
			if (c->contains(id))
				result.insert(c);

	}

	return result;
}

ExprConditionStore::ConditionSet ExprConditionStore::getConditionsOn(ConditionType type, std::string id) const {
	ConditionSet result;

	auto findId = m_conditionsOnVariables.find(id);
	if (findId != m_conditionsOnVariables.end())
	{
		auto find = findId->second.find(type);
		if (find != findId->second.end())
		{
			for (auto const& c: find->second)
				if (c->contains(id))
					result.insert(c);
		}
	}

	return result;
}


void ExprConditionStore::removeConditions(ConditionType type, std::string id) {
	auto& exprs = m_conditions[type];
	for (auto it = exprs.begin(), end = exprs.end(); it != end; )
	{
		if ((**it).contains(id))
			it = exprs.erase(it);
		else
			++it;
	}
	m_conditionsOnVariables.erase(id);
}

void ExprConditionStore::removeConditions(ConditionType type) {
	m_conditions.erase(type);
	for (auto& pair: m_conditionsOnVariables)
		pair.second.erase(type);
}

void ExprConditionStore::clear()
{
	m_conditions.clear();
}

}



