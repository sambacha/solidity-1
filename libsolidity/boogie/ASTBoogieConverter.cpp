#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <libsolidity/boogie/AssignHelper.h>
#include <libsolidity/boogie/ASTBoogieConverter.h>
#include <libsolidity/boogie/ASTBoogieExpressionConverter.h>
#include <libsolidity/boogie/ASTBoogieUtils.h>
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/StoragePtrHelper.h>
#include <libsolidity/analysis/NameAndTypeResolver.h>
#include <libsolidity/analysis/TypeChecker.h>
#include <libsolidity/ast/TypeProvider.h>
#include <libsolidity/parsing/Parser.h>
#include <liblangutil/ErrorReporter.h>

#include <map>

using namespace std;
using namespace solidity;
using namespace solidity::frontend;
using namespace langutil;

namespace bg = boogie;

namespace solidity
{
namespace frontend
{

bg::Expr::Ref ASTBoogieConverter::convertExpression(Expression const& _node)
{
	ASTBoogieExpressionConverter::Result result = ASTBoogieExpressionConverter(m_context).convert(_node, false);

	m_localDecls.insert(end(m_localDecls), begin(result.newDecls), end(result.newDecls));
	for (auto tcc: result.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
		m_currentBlocks.top()->addStmt(bg::Stmt::assume(tcc));
	for (auto s: result.newStatements)
		m_currentBlocks.top()->addStmt(s);
	for (auto oc: result.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION))
		m_currentBlocks.top()->addStmt(bg::Stmt::assign(
			bg::Expr::id(ASTBoogieUtils::VERIFIER_OVERFLOW),
			bg::Expr::or_(bg::Expr::id(ASTBoogieUtils::VERIFIER_OVERFLOW), bg::Expr::not_(oc))));

	return result.expr;
}

void ASTBoogieConverter::createImplicitConstructor(ContractDefinition const& _node)
{
	m_context.addGlobalComment("\nDefault constructor");

	m_localDecls.clear();

	// Create a new error reporter to be able to recover
	ErrorList errorList;
	ErrorReporter errorReporter(errorList);
	ErrorReporter* originalErrReporter = m_context.errorReporter();
	m_context.errorReporter() = &errorReporter;

	// Include preamble
	m_currentBlocks.push(bg::Block::block());
	constructorPreamble();

	// Print errors related to the function
	m_context.printErrors(cerr);
	// Restore error reporter
	m_context.errorReporter() = originalErrReporter;

	// Add function body if there were no errors
	vector<bg::Block::Ref> blocks;
	if (Error::containsOnlyWarnings(errorList))
		blocks.push_back(m_currentBlocks.top());
	else
		m_context.reportWarning(&_node, "Errors while inlining base constructor(s) into implicit constructor, will be skipped");
	m_currentBlocks.pop();
	solAssert(m_currentBlocks.empty(), "Non-empty stack of blocks at the end of function.");

	string funcName = ASTBoogieUtils::getConstructorName(&_node);

	// Input parameters
	std::vector<bg::Binding> params {
		{m_context.boogieThis()->getRefTo(), m_context.boogieThis()->getType() }, // this
		{m_context.boogieMsgSender()->getRefTo(), m_context.boogieMsgSender()->getType() }, // msg.sender
		{m_context.boogieMsgValue()->getRefTo(), m_context.boogieMsgValue()->getType() } // msg.value
	};

	// Create the procedure
	auto procDecl = bg::Decl::procedure(funcName, params, {}, m_localDecls, blocks);
	for (auto invar: m_context.currentContractInvars())
	{
		auto attrs = ASTBoogieUtils::createAttrs(_node.location(), "State variable initializers might violate invariant '" + invar.exprStr + "'.", *m_context.currentScanner());
		procDecl->getEnsures().push_back(bg::Specification::spec(invar.expr, attrs));
	}
	// Overflow condition for the code comes first because if there are more errors, this one gets reported
	if (m_context.overflow())
	{
		auto noOverflow = bg::Expr::not_(bg::Expr::id(ASTBoogieUtils::VERIFIER_OVERFLOW));
		procDecl->getRequires().push_back(bg::Specification::spec(noOverflow,
				ASTBoogieUtils::createAttrs(_node.location(), "An overflow can occur before calling function", *m_context.currentScanner())));
		procDecl->getEnsures().push_back(bg::Specification::spec(noOverflow,
				ASTBoogieUtils::createAttrs(_node.location(), "Function can terminate with overflow", *m_context.currentScanner())));
	}
	auto attrs = ASTBoogieUtils::createAttrs(_node.location(),  _node.name() + "::[implicit_constructor]", *m_context.currentScanner());
	procDecl->addAttrs(attrs);

	if (!Error::containsOnlyWarnings(errorList))
		procDecl->addAttr(bg::Attr::attr("skipped"));

	// Havoc state vars if skipped
	if (!Error::containsOnlyWarnings(errorList))
		for (auto contract: m_context.currentContract()->annotation().linearizedBaseContracts)
			for (auto sv: ASTNode::filteredNodes<VariableDeclaration>(contract->subNodes()))
				if (!sv->isConstant())
					procDecl->getModifies().push_back(m_context.mapDeclName(*sv));

	if (_node.isLibrary()) // Inline for library so that it does not appear in output
		procDecl->addAttr(bg::Attr::attr("inline", 1));
	m_context.addDecl(procDecl);
}

void ASTBoogieConverter::constructorPreamble()
{
	// assume(this.balance >= 0)
	m_currentBlocks.top()->addStmt(bg::Stmt::assume(
			ASTBoogieUtils::encodeArithBinaryOp(m_context, nullptr, Token::GreaterThanOrEqual,
					bg::Expr::arrsel(m_context.boogieBalance()->getRefTo(), m_context.boogieThis()->getRefTo()),
					m_context.intLit(0, 256), 256, false).expr));

	// Initialize state variables first, must be done for
	// base class members as well
	for (auto contract: m_context.currentContract()->annotation().linearizedBaseContracts)
		for (auto sv: ASTNode::filteredNodes<VariableDeclaration>(contract->subNodes()))
			initializeStateVar(*sv);

	int pushedScopes = 0;
	// First initialize the arguments from derived to base
	for (auto base: m_context.currentContract()->annotation().linearizedBaseContracts)
	{
		if (base == m_context.currentContract())
			continue; // Only include base statements, not ours

		// Check if base has a constructor
		FunctionDefinition const* baseConstr = nullptr;
		for (auto fndef: ASTNode::filteredNodes<FunctionDefinition>(base->subNodes()))
			if (fndef->isConstructor())
				baseConstr = fndef;
		if (!baseConstr)
			continue;

		m_context.pushExtraScope(baseConstr, util::toString(m_context.nextId()));
		pushedScopes++;
		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Arguments for " + base->name()));

		// Try to get the argument list (from either inheritance specifiers or modifiers)
		std::vector<ASTPointer<Expression>> const* argsList = nullptr;
		auto constrArgs = m_context.currentContract()->annotation().baseConstructorArguments.find(baseConstr);
		if (constrArgs != m_context.currentContract()->annotation().baseConstructorArguments.end())
		{
			if (auto ispec = dynamic_cast<InheritanceSpecifier const*>(constrArgs->second))
				argsList = ispec->arguments(); // Inheritance specifier
			else if (auto mspec = dynamic_cast<ModifierInvocation const*>(constrArgs->second))
				argsList = mspec->arguments(); // Modifier invocation
		}

		// Introduce and assign local variables for arguments
		for (unsigned long i = 0; i < baseConstr->parameters().size(); i++)
		{
			// Introduce new variable for parameter
			auto param = baseConstr->parameters()[i];
			bg::Decl::Ref constrParam = bg::Decl::variable(m_context.mapDeclName(*param),
					m_context.toBoogieType(param->annotation().type, param.get()));
			m_localDecls.push_back(constrParam);
			// Assign argument
			if (argsList && argsList->size() > i)
			{
				m_currentBlocks.top()->addStmt(bg::Stmt::assign(
						constrParam->getRefTo(),
						convertExpression(*argsList->at(i))));
			}
			else // Or default value
			{
				m_currentBlocks.top()->addStmt(bg::Stmt::assign(
						constrParam->getRefTo(),
						ASTBoogieUtils::defaultValue(param->annotation().type, m_context)));
			}
		}
	}

	// Second, inline the bodies from base to derived
	for (auto it = m_context.currentContract()->annotation().linearizedBaseContracts.rbegin();
			it != m_context.currentContract()->annotation().linearizedBaseContracts.rend(); ++it)
	{
		auto base = *it;
		if (base == m_context.currentContract())
			continue; // Only include base statements, not ours

		// Check if base has a constructor
		FunctionDefinition const* baseConstr = nullptr;
		for (auto fndef: ASTNode::filteredNodes<FunctionDefinition>(base->subNodes()))
			if (fndef->isConstructor())
				baseConstr = fndef;
		if (!baseConstr)
			continue;

		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Inlined constructor for " + base->name() + " starts here"));
		auto m_currentFuncOld = m_currentFunc;
		m_currentFunc = baseConstr;
		m_currentModifier = 0;
		processFuncModifiersAndBody();
		m_currentFunc = m_currentFuncOld;

		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Inlined constructor for " + base->name() + " ends here"));
	}

	// Third, pop all the extra scopes introduced
	for (int i = 0; i < pushedScopes; i++)
		m_context.popExtraScope();
}

void ASTBoogieConverter::createEtherReceiveFunc(ContractDefinition const& _node)
{
	TypePointer tp_uint256 = TypeProvider::integer(256, IntegerType::Modifier::Unsigned);

	vector<bg::Binding> balIncrParams {
		{m_context.boogieThis()->getRefTo(), m_context.boogieThis()->getType() },
		{m_context.boogieMsgValue()->getRefTo(), m_context.boogieMsgValue()->getType() }
	};

	bg::Block::Ref balIncrBlock = bg::Block::block();
	auto gteResult = ASTBoogieUtils::encodeArithBinaryOp(m_context, nullptr, Token::GreaterThanOrEqual, m_context.boogieMsgValue()->getRefTo(), m_context.intLit(0, 256), 256, false);
	balIncrBlock->addStmt(bg::Stmt::assume(gteResult.expr));
	bg::Expr::Ref this_bal = bg::Expr::arrsel(m_context.boogieBalance()->getRefTo(), m_context.boogieThis()->getRefTo());
	if (m_context.encoding() == BoogieContext::Encoding::MOD)
	{
		balIncrBlock->addStmt(bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(this_bal, tp_uint256)));
		balIncrBlock->addStmt(bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(m_context.boogieMsgValue()->getRefTo(), tp_uint256)));
	}
	auto addResult = ASTBoogieUtils::encodeArithBinaryOp(m_context, nullptr, Token::Add, this_bal, m_context.boogieMsgValue()->getRefTo(), 256, false);
	if (m_context.encoding() == BoogieContext::Encoding::MOD)
	{
		balIncrBlock->addStmt(bg::Stmt::comment("Implicit assumption that balances cannot overflow"));
		balIncrBlock->addStmt(bg::Stmt::assume(addResult.cc));
	}
	balIncrBlock->addStmt(bg::Stmt::assign(
					m_context.boogieBalance()->getRefTo(),
					bg::Expr::arrupd(m_context.boogieBalance()->getRefTo(), m_context.boogieThis()->getRefTo(), addResult.expr)));

	bg::ProcDeclRef balIncrProc = bg::Decl::procedure(_node.name() + "_eth_receive", balIncrParams, {}, {}, {balIncrBlock});
	for (auto invar: m_context.currentContractInvars())
	{
		for (auto oc: invar.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION))
		{
			balIncrProc->getRequires().push_back(bg::Specification::spec(oc,
				ASTBoogieUtils::createAttrs(_node.location(), "Overflow in computation of invariant '" + invar.exprStr + "' when entering function.", *m_context.currentScanner())));
			balIncrProc->getEnsures().push_back(bg::Specification::spec(oc,
				ASTBoogieUtils::createAttrs(_node.location(), "Overflow in computation of invariant '" + invar.exprStr + "' at end of function.", *m_context.currentScanner())));
		}
		for (auto tcc: invar.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
		{
			balIncrProc->getRequires().push_back(bg::Specification::spec(tcc,
				ASTBoogieUtils::createAttrs(_node.location(), "Variables in invariant '" + invar.exprStr + "' might be out of range when entering function.", *m_context.currentScanner())));
			balIncrProc->getEnsures().push_back(bg::Specification::spec(tcc,
				ASTBoogieUtils::createAttrs(_node.location(), "Variables in invariant '" + invar.exprStr + "' might be out of range at end of function.", *m_context.currentScanner())));
		}
		balIncrProc->getRequires().push_back(bg::Specification::spec(invar.expr,
				ASTBoogieUtils::createAttrs(_node.location(), "Invariant '" + invar.exprStr + "' might not hold when entering function.", *m_context.currentScanner())));
		balIncrProc->getEnsures().push_back(bg::Specification::spec(invar.expr,
				ASTBoogieUtils::createAttrs(_node.location(), "Invariant '" + invar.exprStr + "' might not hold at end of function.", *m_context.currentScanner())));
	}

	balIncrProc->addAttrs(ASTBoogieUtils::createAttrs(_node.location(), _node.name() + "::[receive_ether_selfdestruct]", *m_context.currentScanner()));

	m_context.addDecl(balIncrProc);
}

void ASTBoogieConverter::initializeStateVar(VariableDeclaration const& _node)
{
	// Constants are inlined
	if (_node.isConstant())
		return;

	string varName = m_context.mapDeclName(_node);
	bg::Expr::Ref varDecl = bg::Expr::id(varName);

	if (_node.value()) // If there is an explicit initializer
	{
		bg::Expr::Ref rhs = convertExpression(*_node.value());
		bg::Expr::Ref lhs = bg::Expr::arrsel(varDecl, m_context.boogieThis()->getRefTo());
		auto ar = AssignHelper::makeAssign(
				AssignHelper::AssignParam{lhs, _node.type(), nullptr},
				AssignHelper::AssignParam{rhs, _node.value()->annotation().type, _node.value().get()},
				Token::Assign, &_node, m_context);
		m_localDecls.insert(m_localDecls.end(), ar.newDecls.begin(), ar.newDecls.end());
		for (auto stmt: ar.newStmts)
			m_currentBlocks.top()->addStmt(stmt);


	}
	else // Use implicit default value
	{
		TypePointer type = _node.type();
		bg::Expr::Ref value = ASTBoogieUtils::defaultValue(type, m_context);
		if (value)
		{
			bg::Stmt::Ref valueAssign = bg::Stmt::assign(varDecl, bg::Expr::arrupd(
					varDecl, m_context.boogieThis()->getRefTo(), value));
			m_currentBlocks.top()->addStmt(valueAssign);

			// Initialize the sum, if there, to default value
			for (auto stmt: m_context.initSumVars(&_node))
				m_currentBlocks.top()->addStmt(stmt);
		}
		else
		{
			m_context.reportWarning(&_node, "Unhandled default value, constructor verification might fail");
		}
	}
}

void ASTBoogieConverter::processSpecificationExpression(ASTPointer<Expression> expr, Parser::SpecificationExpressionInfo const& specInfo,
		ASTNode const& _node, ASTNode const* _scope, BoogieContext::DocTagExpr& result)
{
	// boogie side quantifier info
	std::vector<std::vector<bg::Binding>> bgQuantifierVars;
	std::vector<bg::QuantExpr::Quantifier> bgQuantifierType;

	// Resolve references, using the given scope
	auto scopeDecls = m_context.scopes()[_scope];
	if (specInfo.quantifierList.size() > 0)
	{
		// Resolve types in the variable declaration first
		if (specInfo.arrayId)
			m_context.scopes()[specInfo.arrayId.get()] = m_context.scopes()[_scope];
		NameAndTypeResolver typeResolver(*m_context.globalContext(), m_context.evmVersion(), m_context.scopes(), *m_context.errorReporter());
		if (specInfo.arrayId)
			typeResolver.resolveNamesAndTypes(*specInfo.arrayId);
		// Add all the quantified variables to the scope and create Boogie bindings
		scopeDecls = std::make_shared<DeclarationContainer>(_scope, scopeDecls.get());
		for (size_t i = 0; i < specInfo.quantifierList.size(); ++ i)
		{
			bool isForall = specInfo.isForall[i];
			auto varsBlock = specInfo.quantifierList[i];
			// Resolve types of the variables
			if (typeResolver.resolveNamesAndTypes(*varsBlock))
			{
				auto vars = varsBlock->parameters();
				bgQuantifierVars.push_back({});
				bgQuantifierType.push_back(isForall ? bg::QuantExpr::Forall : bg::QuantExpr::Exists);
				for (auto varDecl: vars)
				{
					scopeDecls->registerDeclaration(*varDecl);
					auto varName = m_context.mapDeclName(*varDecl);
					auto varType = m_context.toBoogieType(varDecl->type(), varDecl.get());
					auto varExpr = bg::Expr::id(varName);
					bgQuantifierVars.back().push_back({varExpr, varType});
				}
			}
		}
	}
	m_context.scopes()[expr.get()] = scopeDecls;

	NameAndTypeResolver exprResolver(*m_context.globalContext(), m_context.evmVersion(), m_context.scopes(), *m_context.errorReporter());
	if (exprResolver.resolveNamesAndTypes(*expr))
	{
		// Do type checking
		TypeChecker typeChecker(m_context.evmVersion(), *m_context.errorReporter());
		if (typeChecker.checkTypeRequirements(*m_context.currentSource(), *expr))
		{
			// Convert expression to Boogie representation
			auto convResult = ASTBoogieExpressionConverter(m_context).convert(*expr, true);
			// Add index bounds if array is there
			if (specInfo.arrayId)
			{
				std::vector<bg::Expr::Ref> guards;
				solAssert(bgQuantifierType.size() == 1 && bgQuantifierVars.size() == 1, "");
				solAssert(bgQuantifierType.back() == bg::QuantExpr::Forall, "");
				auto arrayType = specInfo.arrayId->annotation().referencedDeclaration->type();
				auto arrayTypeSpec = dynamic_cast<ArrayType const*>(arrayType);
				if (arrayTypeSpec)
				{
					auto arrayBaseType = arrayTypeSpec->baseType();
					auto arrayBaseTypeBg = m_context.toBoogieType(arrayBaseType, specInfo.arrayId.get());
					auto arrayExpr = ASTBoogieExpressionConverter(m_context).convert(*specInfo.arrayId, false).expr;
					auto arrayLocation = arrayTypeSpec->location();
					if (arrayLocation == DataLocation::Memory || arrayLocation == DataLocation::CallData)
						arrayExpr = m_context.getMemArray(arrayExpr, arrayBaseTypeBg);
					auto arrayLength = m_context.getArrayLength(arrayExpr, arrayBaseTypeBg);
					auto const& bindings = bgQuantifierVars.back();
					for (auto const& b: bindings)
					{
						guards.push_back(bg::Expr::lte(bg::Expr::intlit((unsigned) 0), b.id));
						guards.push_back(bg::Expr::lt(b.id, arrayLength));
					}
					auto guard = bg::Expr::and_(guards);
					convResult.expr = bg::Expr::impl(guard, convResult.expr);
				}
				else
					m_context.reportError(&_node, "Specification of an array property must be over an array");
			}

			// Add quantifiers if necessary
			while (bgQuantifierType.size() > 0)
			{
				auto type = bgQuantifierType.back();
				auto const& bindings = bgQuantifierVars.back();

				// Add any type-checking conditions
				std::vector<bg::Expr::Ref> bindingConditions;
				std::vector<bg::Expr::Ref> bindingTCCs;
				// First, we cannot handle OCs, issue warning
				if (!convResult.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION).empty())
				{
					convResult.conditions.removeConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION);
					m_context.reportWarning(&_node, "Annotation generates overflow checking conditions but they are not supported within quantifiers.");
				}
				for (auto expr: bindings)
				{
					auto varExpr = std::dynamic_pointer_cast<boogie::VarExpr const>(expr.id);
					auto conditions = convResult.conditions.getConditionsContaining(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION, varExpr->getName());
					bindingConditions.insert(bindingConditions.end(), conditions.begin(), conditions.end());
					auto tccs = convResult.conditions.getConditionsOn(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION, varExpr->getName());
					bindingTCCs.insert(bindingTCCs.end(), tccs.begin(), tccs.end());
					convResult.conditions.removeConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION, varExpr->getName());
				}

				switch (type)
				{
				case bg::QuantExpr::Forall:
					if (bindingConditions.size() || bindingTCCs.size())
					{
						auto conditions = bg::Expr::and_(bindingConditions);
						auto tccs = bg::Expr::and_(bindingTCCs);
						convResult.expr = bg::Expr::and_(conditions, convResult.expr);
						convResult.expr = bg::Expr::impl(tccs, convResult.expr);
					}
					convResult.expr = bg::Expr::forall(bindings, convResult.expr);
					break;
				case bg::QuantExpr::Exists:
					if (bindingConditions.size() || bindingTCCs.size())
					{
						auto conditions = bg::Expr::and_(bindingConditions);
						auto tccs = bg::Expr::and_(bindingTCCs);
						convResult.expr = bg::Expr::and_(conditions, convResult.expr);
						convResult.expr = bg::Expr::and_(tccs, convResult.expr);
					}
					convResult.expr = bg::Expr::exists(bindings, convResult.expr);
					break;
				}

				bgQuantifierType.pop_back();
				bgQuantifierVars.pop_back();
			}

			result.expr = convResult.expr;
			result.conditions = convResult.conditions;

			// Report unsupported cases (side effects)
			if (!convResult.newStatements.empty())
				m_context.reportError(&_node, "Annotation expression introduces intermediate statements");
			if (!convResult.newDecls.empty())
				m_context.reportError(&_node, "Annotation expression introduces intermediate declarations");

		}
	}
}

bool ASTBoogieConverter::parseExpr(string exprStr, ASTNode const& _node, ASTNode const* _scope, BoogieContext::DocTagExpr& result)
{
	// We temporarily replace the error reporter in the context, because the locations
	// are pointing to positions in the docstring
	ErrorList errorList;
	ErrorReporter errorReporter(errorList);

	ErrorReporter* originalErrReporter = m_context.errorReporter();
	m_context.errorReporter() = &errorReporter;

	try
	{
		// solidity side quantifier info
		Parser::SpecificationExpressionInfo specInfo;

		// Parse
		CharStream exprStream(exprStr, "Annotation");
		Parser parser(*m_context.errorReporter(), m_context.evmVersion());
		auto scanner = std::make_shared<Scanner>(exprStream);
		ASTPointer<Expression> expr = parser.parseSpecificationExpression(scanner, specInfo);
		if (!expr)
			throw langutil::FatalError();

		// Process the expression (add quantifiers and stuff)
		processSpecificationExpression(expr, specInfo, _node, _scope, result);
		result.exprStr = exprStr;
		result.exprSol = expr;
	}
	catch (langutil::FatalError const&)
	{
		m_context.reportError(&_node, "Error while parsing annotation.");
	}

	// Print errors relating to the expression string
	m_context.printErrors(cerr);

	// Restore error reporter
	m_context.errorReporter() = originalErrReporter;
	// Add a single error in the original reporter if there were errors
	if (!Error::containsOnlyWarnings(errorList))
	{
		m_context.reportError(&_node, "Error(s) while translating annotation for node");
		return false;
	}
	else if (errorList.size() > 0)
	{
		m_context.reportWarning(&_node, "Warning(s) while translating annotation for node");
	}
	return true;
}

bool ASTBoogieConverter::parseSpecificationCasesExpr(string exprStr, ASTNode const& _node, ASTNode const* _scope, BoogieContext::DocTagExpr& result)
{
	// We temporarily replace the error reporter in the context, because the locations
	// are pointing to positions in the docstring
	ErrorList errorList;
	ErrorReporter errorReporter(errorList);

	ErrorReporter* originalErrReporter = m_context.errorReporter();
	m_context.errorReporter() = &errorReporter;

	try
	{
		// Setup
		CharStream exprStream(exprStr, "Annotation");
		Parser parser(*m_context.errorReporter(), m_context.evmVersion());
		auto scanner = std::make_shared<Scanner>(exprStream);

		// Parse the cases
		std::vector<Parser::SpecificationCase> specCases;
		parser.parseSpecificationExpression(scanner, specCases);

		// Convert cases to boogie
		std::vector<boogie::Expr::Ref> specCasesBoogie;
		std::vector<boogie::Expr::Ref> specCasesPreBoogie;
		for (auto const& specCase: specCases)
		{
			if (!specCase.precondition || !specCase.postcondition)
				throw langutil::FatalError();

			BoogieContext::DocTagExpr pre;
			processSpecificationExpression(specCase.precondition, specCase.preconditionInfo, _node, _scope, pre);
			auto preOld = boogie::Expr::old(pre.expr);
			specCasesPreBoogie.push_back(preOld);

			/// TODO: tccs, ocs?

			BoogieContext::DocTagExpr post;
			processSpecificationExpression(specCase.postcondition, specCase.postconditionInfo, _node, _scope, post);

			auto specCaseBoogie = boogie::Expr::impl(preOld, post.expr);
			specCasesBoogie.push_back(specCaseBoogie);

			result.conditions.addConditions(result.conditions);
		}

		// Add another case: !(case_1 || ... || case_n) => noop
		std::vector<bg::Expr::Ref> noopVars;
		auto casesCovered = boogie::Expr::or_(specCasesPreBoogie);
		// Else branch: balance stays the same
		auto balanceEq = bg::Expr::eq(m_context.boogieBalance()->getRefTo(), bg::Expr::old(m_context.boogieBalance()->getRefTo()));
		noopVars.push_back(balanceEq);
		// Else branch: state variables stay the same
		for (auto contract: m_context.currentContract()->annotation().linearizedBaseContracts)
		{
			for (auto varDecl: ASTNode::filteredNodes<VariableDeclaration>(contract->subNodes()))
			{
				if (varDecl->isConstant())
					continue;
				auto varId = bg::Expr::id(m_context.mapDeclName(*varDecl));
				auto varThis = bg::Expr::arrsel(varId, m_context.boogieThis()->getRefTo());
				auto varEq = bg::Expr::eq(varThis, bg::Expr::old(varThis));
				noopVars.push_back(varEq);
			}
		}
		auto noop = bg::Expr::and_(noopVars);
		specCasesBoogie.push_back(bg::Expr::or_(casesCovered, noop));

		// Construct the postcondition:
		// conjunction[case] old(case.pre) => case.post
        //
		result.expr = boogie::Expr::and_(specCasesBoogie);
		result.exprStr = exprStr;
		result.exprSol = nullptr;
	}
	catch (langutil::FatalError const&)
	{
		m_context.reportError(&_node, "Error while parsing annotation.");
	}

	// Print errors relating to the expression string
	m_context.printErrors(cerr);

	// Restore error reporter
	m_context.errorReporter() = originalErrReporter;
	// Add a single error in the original reporter if there were errors
	if (!Error::containsOnlyWarnings(errorList))
	{
		m_context.reportError(&_node, "Error(s) while translating annotation for node");
		return false;
	}
	else if (errorList.size() > 0)
	{
		m_context.reportWarning(&_node, "Warning(s) while translating annotation for node");
	}
	return true;
}

std::vector<BoogieContext::DocTagExpr> ASTBoogieConverter::getExprsFromDocTags(ASTNode const& _node, StructurallyDocumentedAnnotation const& _annot,
		ASTNode const* _scope, vector<string> const& _tags)
{
	std::vector<BoogieContext::DocTagExpr> exprs;
	for (auto tag: _tags)
	{
		for (auto docTag: _annot.docTags)
		{
			if (docTag.first == "notice" && boost::starts_with(docTag.second.content, tag)) // Find expressions with the given tag
			{
				BoogieContext::DocTagExpr expr;
				if (tag == ASTBoogieUtils::DOCTAG_SPECIFICATION_CASES)
				{
					if (parseSpecificationCasesExpr(docTag.second.content.substr(tag.length() + 1), _node, _scope, expr))
						exprs.push_back(expr);
				}
				else
				{
					if (parseExpr(docTag.second.content.substr(tag.length() + 1), _node, _scope, expr))
						exprs.push_back(expr);
				}
			}
		}
	}
	return exprs;
}

bool ASTBoogieConverter::includeContractInvars(StructurallyDocumentedAnnotation const& _annot)
{
	for (auto docTag: _annot.docTags)
		if (docTag.first == "notice" && boost::starts_with(docTag.second.content, ASTBoogieUtils::DOCTAG_CONTRACT_INVARS_INCLUDE))
			return true;

	return false;
}

bool ASTBoogieConverter::collectEmitsSpecs(FunctionDefinition const& _node)
{
	// TODO: this is a duplication, EmitsChecker already has this information

	m_currentEmits.clear();
	// Collect all the events from the docTags and enable tracking
	for (auto docTag: _node.annotation().docTags)
	{
		if (docTag.first == "notice" && boost::starts_with(docTag.second.content, ASTBoogieUtils::DOCTAG_EMITS))
		{
			bool ok = false;
			string eventName = docTag.second.content.substr(ASTBoogieUtils::DOCTAG_EMITS.length() + 1);
			boost::trim(eventName);

			DeclarationContainer const* container = m_context.scopes()[&_node].get();
			while (container)
			{
				if (container->declarations().find(eventName) != container->declarations().end())
					for (auto const decl: container->declarations().at(eventName))
						if (auto eventDecl = dynamic_cast<EventDefinition const*>(decl))
						{
							m_currentEmits.insert(eventDecl);
							m_context.enableEventDataTrackingFor(eventDecl);
							ok = true;
						}

				container = container->enclosingContainer();
			}

			if (!ok)
			{
				m_context.reportError(&_node, "No event found with name '" + eventName + "'.");
				return false;
			}
		}
	}

	return true;
}

Declaration const* ASTBoogieConverter::getModifiesBase(Expression const* expr)
{
	if (auto id = dynamic_cast<Identifier const*>(expr))
	{
		return id->annotation().referencedDeclaration;
	}
	else if (auto ma = dynamic_cast<MemberAccess const*>(expr))
	{
		auto decl = dynamic_cast<VariableDeclaration const*>(ma->annotation().referencedDeclaration);
		if (decl && decl->isStateVariable())
			return decl;
		else return getModifiesBase(&ma->expression());
	}
	else if (auto idx = dynamic_cast<IndexAccess const*>(expr))
	{
		return getModifiesBase(&idx->baseExpression());
	}
	return nullptr;
}

bool ASTBoogieConverter::isBaseVar(bg::Expr::Ref expr)
{
	if (auto exprArrSel = dynamic_pointer_cast<bg::ArrSelExpr const>(expr))
	{
		// Base is reached when it is a variable indexed with 'this'
		auto idxAsId = dynamic_pointer_cast<bg::VarExpr const>(exprArrSel->getIdx());
		if (dynamic_pointer_cast<bg::VarExpr const>(exprArrSel->getBase()) &&
				idxAsId && idxAsId->getName() == m_context.boogieThis()->getName())
		{
			return true;
		}
	}
	return false;
}

bg::Expr::Ref ASTBoogieConverter::replaceBaseVar(bg::Expr::Ref expr, bg::Expr::Ref value)
{
	if (isBaseVar(expr))
		return value;
	if (auto exprSel = dynamic_pointer_cast<bg::SelExpr const>(expr))
		return exprSel->replaceBase(replaceBaseVar(exprSel->getBase(), value));
	solAssert(false, "Base could not be replaced");
	return expr;
}

void ASTBoogieConverter::addModifiesSpecs(FunctionDefinition const& _node, bg::ProcDeclRef procDecl)
{
	// Modifies specifier
	struct ModSpec {
		bg::Expr::Ref cond;   // Condition
		bg::Expr::Ref target; // Target (identifier/selector)

		ModSpec() {}
		ModSpec(bg::Expr::Ref cond, bg::Expr::Ref target) : cond(cond), target(target) {}
	};

	map<Declaration const*, list<ModSpec>> modSpecs; // Modifies specifier for each variable
	list<ModSpec> balanceModSpecs; // Modifies specifiers for global balances
	bool canModifyAll = false;

	for (auto docTag: _node.annotation().docTags)
	{
		if (docTag.first == "notice" && boost::starts_with(docTag.second.content, ASTBoogieUtils::DOCTAG_MODIFIES))
		{
			if (boost::algorithm::trim_copy(docTag.second.content) == ASTBoogieUtils::DOCTAG_MODIFIES_ALL)
			{
				canModifyAll = true;
				continue; // Continue to parse the rest to catch syntax errors
			}
			size_t targetEnd = docTag.second.content.length();
			bg::Expr::Ref condExpr = bg::Expr::true_();
			// Check if there is a condition part
			size_t condStart = docTag.second.content.find(ASTBoogieUtils::DOCTAG_MODIFIES_COND);
			if (condStart != string::npos)
			{
				targetEnd = condStart;
				// Parse the condition
				BoogieContext::DocTagExpr cond;
				if (parseExpr(docTag.second.content.substr(condStart + ASTBoogieUtils::DOCTAG_MODIFIES_COND.length()), _node, &_node, cond))
					condExpr = bg::Expr::old(cond.expr);
			}
			// Parse the target (identifier/selector)
			BoogieContext::DocTagExpr target;
			size_t targetStart = ASTBoogieUtils::DOCTAG_MODIFIES.length() + 1;
			if (parseExpr(docTag.second.content.substr(targetStart, targetEnd - targetStart + 1), _node, &_node, target))
			{
				auto memAccExpr = dynamic_cast<MemberAccess const*>(target.exprSol.get());
				if (memAccExpr && memAccExpr->memberName() == ASTBoogieUtils::BALANCE.solidity &&
						memAccExpr->expression().annotation().type->category() == Type::Category::Address)
					balanceModSpecs.push_back(ModSpec(condExpr, dynamic_pointer_cast<bg::ArrSelExpr const>(target.expr)->getIdx()));
				else if (auto varDecl = dynamic_cast<VariableDeclaration const*>(getModifiesBase(target.exprSol.get())))
				{
					if (varDecl->isStateVariable())
						modSpecs[varDecl].push_back(ModSpec(condExpr, target.expr));
					else
						m_context.reportWarning(&_node, "Modifies specification for non-state variable '" + varDecl->name() + "' ignored");
				}
				else
					m_context.reportError(&_node, "Invalid target expression for modifies specification");
			}
		}
	}

	if (canModifyAll && !modSpecs.empty())
		m_context.reportWarning(&_node, "Modifies all was given, other modifies specifications are ignored");


	// Global balances
	if (m_context.modAnalysis() && !canModifyAll)
	{
		// Build up expression recursively
		bg::Expr::Ref expr = bg::Expr::old(m_context.boogieBalance()->getRefTo());
		for (auto modSpec: balanceModSpecs)
		{
			expr = bg::Expr::cond(modSpec.cond,
					bg::Expr::arrupd(expr, modSpec.target, bg::Expr::arrsel(m_context.boogieBalance()->getRefTo(), modSpec.target)),
					expr);
		}
		expr = bg::Expr::eq(m_context.boogieBalance()->getRefTo(), expr);
		procDecl->getEnsures().push_back(bg::Specification::spec(expr,
								ASTBoogieUtils::createAttrs(_node.location(), "Function might modify balances illegally", *m_context.currentScanner())));
		m_context.warnForBalances();
	}

	// State vars
	if (m_context.modAnalysis() && !_node.isConstructor() && !canModifyAll)
	{
		// Linearized base contracts include the current contract as well
		for (auto contract: m_context.currentContract()->annotation().linearizedBaseContracts)
		{
			for (auto varDecl: ASTNode::filteredNodes<VariableDeclaration>(contract->subNodes()))
			{
				if (varDecl->isConstant())
					continue;
				auto varId = bg::Expr::id(m_context.mapDeclName(*varDecl));
				auto varThis = bg::Expr::arrsel(varId, m_context.boogieThis()->getRefTo());

				// Build up expression recursively
				bg::Expr::Ref expr = bg::Expr::old(varThis);

				for (auto modSpec: modSpecs[varDecl])
				{
					if (isBaseVar(modSpec.target))
					{
						expr = bg::Expr::cond(modSpec.cond, varThis, expr);
					}
					else
					{
						auto repl = replaceBaseVar(modSpec.target, expr);
						auto write = bg::Expr::selectToUpdate(repl, modSpec.target);
						expr = bg::Expr::cond(modSpec.cond, write, expr);
					}
				}

				expr = bg::Expr::eq(varThis, expr);
				string varName = varDecl->name();
				if (m_context.currentContract()->annotation().linearizedBaseContracts.size() > 1)
					varName = contract->name() + "::" + varName;
				procDecl->getEnsures().push_back(bg::Specification::spec(expr,
						ASTBoogieUtils::createAttrs(_node.location(), "Function might modify '" + varName + "' illegally", *m_context.currentScanner())));
			}
		}
	}
}

void ASTBoogieConverter::processFuncModifiersAndBody()
{
	if (m_currentModifier < m_currentFunc->modifiers().size()) // We still have modifiers
	{
		auto modifier = m_currentFunc->modifiers()[m_currentModifier];
		auto modifierDecl = dynamic_cast<ModifierDefinition const*>(modifier->name()->annotation().referencedDeclaration);

		if (modifierDecl)
		{
			m_context.pushExtraScope(modifierDecl, util::toString(m_context.nextId()) + util::toString(m_currentModifier));

			string oldReturnLabel = m_currentReturnLabel;
			m_currentReturnLabel = "$return" + to_string(m_nextReturnLabelId);
			++m_nextReturnLabelId;
			m_currentBlocks.top()->addStmt(bg::Stmt::comment("Inlined modifier " + modifierDecl->name() + " starts here"));

			// Introduce and assign local variables for modifier arguments
			if (modifier->arguments())
			{
				for (unsigned long i = 0; i < modifier->arguments()->size(); ++i)
				{
					auto paramDecls = modifierDecl->parameters()[i];
					bg::Decl::Ref modifierParam = bg::Decl::variable(m_context.mapDeclName(*paramDecls),
							m_context.toBoogieType(modifierDecl->parameters()[i]->annotation().type, paramDecls.get()));
					m_localDecls.push_back(modifierParam);
					bg::Expr::Ref modifierArg = convertExpression(*modifier->arguments()->at(i));
					m_currentBlocks.top()->addStmt(bg::Stmt::assign(modifierParam->getRefTo(), modifierArg));
				}
			}
			modifierDecl->body().accept(*this);
			m_currentBlocks.top()->addStmt(bg::Stmt::label(m_currentReturnLabel));
			m_currentBlocks.top()->addStmt(bg::Stmt::comment("Inlined modifier " + modifierDecl->name() + " ends here"));
			m_currentReturnLabel = oldReturnLabel;
			m_context.popExtraScope();
		}
		// Base constructor arguments can skipped, calls to base constructors are inlined
		else if (dynamic_cast<ContractDefinition const*>(modifier->name()->annotation().referencedDeclaration))
		{
			m_currentModifier++;
			processFuncModifiersAndBody();
			m_currentModifier--;
		}
		else
		{
			m_context.reportError(modifier.get(), "Unsupported modifier invocation");
		}
	}
	else if (m_currentFunc->isImplemented()) // We reached the function
	{
		if (!m_currentFunc->modifiers().empty())
			m_context.pushExtraScope(&m_currentFunc->body(), util::toString(m_context.nextId()));
		string oldReturnLabel = m_currentReturnLabel;
		m_currentReturnLabel = "$return" + to_string(m_nextReturnLabelId);
		++m_nextReturnLabelId;
		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Function body starts here"));
		m_currentFunc->body().accept(*this);
		m_currentBlocks.top()->addStmt(bg::Stmt::label(m_currentReturnLabel));
		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Function body ends here"));
		m_currentReturnLabel = oldReturnLabel;
		if (!m_currentFunc->modifiers().empty())
			m_context.popExtraScope();
	}
}

ASTBoogieConverter::ASTBoogieConverter(BoogieContext& context) :
				m_context(context),
				m_currentFunc(nullptr),
				m_currentModifier(0),
				m_currentRet(nullptr),
				m_nextReturnLabelId(0)
{
}

// ---------------------------------------------------------------------------
//         Visitor methods for top-level nodes and declarations
// ---------------------------------------------------------------------------

bool ASTBoogieConverter::visit(SourceUnit const& _node)
{
	rememberScope(_node);

	m_context.setCurrentSource(&_node);

	// Boogie programs are flat, source units do not appear explicitly
	m_context.addGlobalComment("\n------- Source: " + *_node.annotation().path + " -------");
	return true; // Simply apply visitor recursively
}

bool ASTBoogieConverter::visit(PragmaDirective const& _node)
{
	rememberScope(_node);

	// Pragmas are only included as comments
	m_context.addGlobalComment("Pragma: " + boost::algorithm::join(_node.literals(), ""));
	return false;
}

bool ASTBoogieConverter::visit(ImportDirective const& _node)
{
	rememberScope(_node);
	return false;
}

bool ASTBoogieConverter::visit(ContractDefinition const& _node)
{
	rememberScope(_node);

	// Set current contract (updates this/super)
	m_context.setCurrentContract(&_node);

	// Boogie programs are flat, contracts do not appear explicitly
	m_context.addGlobalComment("\n------- Contract: " + _node.name() + " -------");

	// Process contract invariants
	m_context.currentContractInvars().clear();

	for (auto invar: getExprsFromDocTags(_node, _node.annotation(), &_node, { ASTBoogieUtils::DOCTAG_CONTRACT_INVAR }))
	{
		m_context.addGlobalComment("Contract invariant: " + invar.exprStr);
		m_context.currentContractInvars().push_back(invar);
	}

	// Process inheritance specifiers (not included in subNodes)
	for (auto ispec: _node.baseContracts())
		ispec->accept(*this);

	// Process any event definitions
	for (auto event: ASTNode::filteredNodes<EventDefinition>(_node.subNodes()))
		processEventDefinition(*event);

	// Process subnodes
	for (auto sn: _node.subNodes())
		sn->accept(*this);

	// If no constructor exists, create an implicit one
	bool hasConstructor = false;
	for (auto fn: ASTNode::filteredNodes<FunctionDefinition>(_node.subNodes()))
	{
		if (fn->isConstructor())
		{
			hasConstructor = true;
			break;
		}
	}
	if (!hasConstructor)
		createImplicitConstructor(_node);

	// Create Ether receiving function (selfdestruct)
	if (!m_context.currentContractInvars().empty())
		createEtherReceiveFunc(_node);

	// Rest current contract (removes this, super)
	m_context.setCurrentContract(nullptr);

	return false;
}

bool ASTBoogieConverter::visit(InheritanceSpecifier const& _node)
{
	rememberScope(_node);
	// Boogie programs are flat, inheritance does not appear explicitly
	m_context.addGlobalComment("Inherits from: " + boost::algorithm::join(_node.name().namePath(), "#"));
	return false;
}

bool ASTBoogieConverter::visit(UsingForDirective const& _node)
{
	rememberScope(_node);

	// Nothing to do with using for directives, calls to functions are resolved in the AST
	string libraryName = _node.libraryName().annotation().type->toString();
	string typeName = _node.typeName() ? _node.typeName()->annotation().type->toString() : "*";
	m_context.addGlobalComment("Using " + libraryName + " for " + typeName);
	return false;
}

bool ASTBoogieConverter::visit(StructDefinition const& _node)
{
	rememberScope(_node);

	m_context.addGlobalComment("\n------- Struct " + _node.name() + " -------");
	m_context.addGlobalComment("Storage");
	m_context.getStructType(&_node, DataLocation::Storage);
	m_context.getStructConstructor(&_node);

	m_context.addGlobalComment("Memory");
	// Define type for memory
	bg::TypeDeclRef structMemType = m_context.getStructType(&_node, DataLocation::Memory);
	// Create mappings for each member (only for memory structs)
	for (auto member: _node.members())
	{
		bg::TypeDeclRef memberType = nullptr;
		// Nested structures
		if (member->type()->category() == Type::Category::Struct)
		{
			auto structTp = dynamic_cast<StructType const*>(member->type());
			memberType = m_context.getStructType(&structTp->structDefinition(), DataLocation::Memory);
		}
		else // Other types
			memberType = m_context.toBoogieType(TypeProvider::withLocationIfReference(DataLocation::Memory, member->type()), member.get());

		auto attrs = ASTBoogieUtils::createAttrs(member->location(), member->name(), *m_context.currentScanner());
		auto memberDecl = bg::Decl::variable(m_context.mapDeclName(*member), bg::Decl::arraytype(structMemType, memberType));
		memberDecl->addAttrs(attrs);
		m_context.addGlobalComment("Member " + member->name());
		m_context.addDecl(memberDecl);
	}
	m_context.addGlobalComment("\n------- End of struct " + _node.name() + " -------");

	return false;
}

bool ASTBoogieConverter::visit(EnumDefinition const& _node)
{
	rememberScope(_node);
	m_context.addGlobalComment("Enum definition " + _node.name() + " mapped to int");
	return false;
}

bool ASTBoogieConverter::visit(EnumValue const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: EnumValue");
	return false;
}

bool ASTBoogieConverter::visit(ParameterList const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: ParameterList");
	return false;
}

bool ASTBoogieConverter::visit(FunctionDefinition const& _node)
{
	rememberScope(_node);

	// Solidity functions are mapped to Boogie procedures
	m_currentFunc = &_node;

	// Type to pass around
	TypePointer tp_uint256 = TypeProvider::integer(256, IntegerType::Modifier::Unsigned);

	// Input parameters
	vector<bg::Binding> params;
	// Globally available stuff
	params.push_back({m_context.boogieThis()->getRefTo(), m_context.boogieThis()->getType() });// this
	params.push_back({m_context.boogieMsgSender()->getRefTo(), m_context.boogieMsgSender()->getType() });// msg.sender
	params.push_back({m_context.boogieMsgValue()->getRefTo(), m_context.boogieMsgValue()->getType() }); // msg.value
	// Add original parameters of the function
	for (auto par: _node.parameters())
		params.push_back({bg::Expr::id(m_context.mapDeclName(*par)), m_context.toBoogieType(par->type(), par.get())});

	// Return values
	vector<bg::Binding> rets;
	vector<bg::Expr::Ref> retIds;
	for (auto ret: _node.returnParameters())
	{
		bg::Expr::Ref retId = bg::Expr::id(m_context.mapDeclName(*ret));
		bg::TypeDeclRef retType = m_context.toBoogieType(ret->type(), ret.get());
		retIds.push_back(retId);
		rets.push_back({retId, retType});
	}

	// Boogie treats return as an assignment to the return variable(s)
	if (_node.returnParameters().empty())
		m_currentRet = nullptr;
	else if (_node.returnParameters().size() == 1)
		m_currentRet = retIds[0];
	else
		m_currentRet = bg::Expr::tuple(retIds);

	// Collect emits specs before processing body
	collectEmitsSpecs(_node);

	// Create a new error reporter to be able to recover
	ErrorList errorList;
	ErrorReporter errorReporter(errorList);
	ErrorReporter* originalErrReporter = m_context.errorReporter();
	m_context.errorReporter() = &errorReporter;

	// Convert function body, collect result
	m_localDecls.clear();
	// Create new empty block
	m_currentBlocks.push(bg::Block::block());
	// Basic non-aliasing between parameters and newly allocated stuff
	for (auto par: _node.parameters())
	{
		if (auto parRef = dynamic_cast<ReferenceType const*>(par->annotation().type))
		{
			if (parRef->dataStoredIn(DataLocation::Memory))
			{
				m_currentBlocks.top()->addStmt(bg::Stmt::assume(bg::Expr::lt(
						bg::Expr::id(m_context.mapDeclName(*par)),
						m_context.getAllocCounter()->getRefTo())));
			}
		}
	}
	// Assume msg.sender is != 0
	m_currentBlocks.top()->addStmt(bg::Stmt::assume(bg::Expr::neq(m_context.boogieMsgSender()->getRefTo(), m_context.intLit(0, 256))));
	// Include constructor preamble
	if (_node.isConstructor())
		constructorPreamble();
	// Payable functions should handle msg.value
	if (_node.isPayable())
	{
		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Update balance received by msg.value"));
		bg::Expr::Ref this_bal = bg::Expr::arrsel(m_context.boogieBalance()->getRefTo(), m_context.boogieThis()->getRefTo());
		bg::Expr::Ref msg_val = m_context.boogieMsgValue()->getRefTo();
		// balance[this] += msg.value
		if (m_context.encoding() == BoogieContext::Encoding::MOD)
		{
			m_currentBlocks.top()->addStmt(bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(this_bal, tp_uint256)));
			m_currentBlocks.top()->addStmt(bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(msg_val, tp_uint256)));
		}
		auto addResult = ASTBoogieUtils::encodeArithBinaryOp(m_context, nullptr, Token::Add, this_bal, msg_val, 256, false);
		if (m_context.encoding() == BoogieContext::Encoding::MOD)
		{
			m_currentBlocks.top()->addStmt(bg::Stmt::comment("Implicit assumption that balances cannot overflow"));
			m_currentBlocks.top()->addStmt(bg::Stmt::assume(addResult.cc));
		}
		m_currentBlocks.top()->addStmt(bg::Stmt::assign(
				m_context.boogieBalance()->getRefTo(),
					bg::Expr::arrupd(m_context.boogieBalance()->getRefTo(), m_context.boogieThis()->getRefTo(), addResult.expr)));
	}

	// Modifiers need to be inlined
	m_currentModifier = 0;
	processFuncModifiersAndBody();

	// Print errors related to the function
	m_context.printErrors(cerr);

	// Restore error reporter
	m_context.errorReporter() = originalErrReporter;


	// Create a separate block for TCCs
	bg::Block::Ref tccAssumes = bg::Block::block();
	tccAssumes->addStmt(bg::Stmt::comment("TCC assumptions"));
	// Add function body if there were no errors and is implemented
	vector<bg::Block::Ref> blocks;
	if (Error::containsOnlyWarnings(errorList))
	{
		if (_node.isImplemented())
		{
			blocks.push_back(tccAssumes);
			blocks.push_back(m_currentBlocks.top());
		}
	}
	else
		m_context.reportWarning(&_node, "Errors while translating function body, will be skipped");

	m_currentBlocks.pop();
	solAssert(m_currentBlocks.empty(), "Non-empty stack of blocks at the end of function.");

	// Disable event tracking since we're done with the body
	m_context.disableEventDataTracking();

	// Get the name of the function
	string funcName = _node.isConstructor() ?
			ASTBoogieUtils::getConstructorName(m_context.currentContract()) :
			m_context.mapDeclName(_node);

	// Create the procedure
	auto procDecl = bg::Decl::procedure(funcName, params, rets, m_localDecls, blocks);

	// Overflow condition for the code comes first because if there are more errors, this one gets reported
	if (m_context.overflow())
	{
		auto noOverflow = bg::Expr::not_(bg::Expr::id(ASTBoogieUtils::VERIFIER_OVERFLOW));
		procDecl->getRequires().push_back(bg::Specification::spec(noOverflow,
				ASTBoogieUtils::createAttrs(_node.location(), "An overflow can occur before calling function", *m_context.currentScanner())));
		procDecl->getEnsures().push_back(bg::Specification::spec(noOverflow,
				ASTBoogieUtils::createAttrs(_node.location(), "Function can terminate with overflow", *m_context.currentScanner())));
	}

	// add invariants as pre/postconditions for: public functions and if explicitly requested
	if (_node.isConstructor() || _node.isPublic() || includeContractInvars(_node.annotation()))
	{
		for (auto invar: m_context.currentContractInvars())
		{
			for (auto oc: invar.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION) )
			{
				procDecl->getRequires().push_back(bg::Specification::spec(oc,
					ASTBoogieUtils::createAttrs(_node.location(), "Overflow in computation of invariant '" + invar.exprStr + "' when entering function.", *m_context.currentScanner())));
				procDecl->getEnsures().push_back(bg::Specification::spec(oc,
					ASTBoogieUtils::createAttrs(_node.location(), "Overflow in computation of invariant '" + invar.exprStr + "' at end of function.", *m_context.currentScanner())));
			}
			for (auto tcc: invar.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
			{
				procDecl->getRequires().push_back(bg::Specification::spec(tcc,
					ASTBoogieUtils::createAttrs(_node.location(), "Variables in invariant '" + invar.exprStr + "' might be out of range when entering function.", *m_context.currentScanner())));
				procDecl->getEnsures().push_back(bg::Specification::spec(tcc,
					ASTBoogieUtils::createAttrs(_node.location(), "Variables in invariant '" + invar.exprStr + "' might be out of range at end of function.", *m_context.currentScanner())));
			}
			if (!_node.isConstructor())
			{
				procDecl->getRequires().push_back(bg::Specification::spec(invar.expr,
					ASTBoogieUtils::createAttrs(_node.location(), "Invariant '" + invar.exprStr + "' might not hold when entering function.", *m_context.currentScanner())));
			}
			procDecl->getEnsures().push_back(bg::Specification::spec(invar.expr,
					ASTBoogieUtils::createAttrs(_node.location(), "Invariant '" + invar.exprStr + "' might not hold at end of function.", *m_context.currentScanner())));
		}
	}

	if (!_node.isConstructor() && !_node.isPublic()) // Non-public functions: inline
		procDecl->addAttr(bg::Attr::attr("inline", 1));

	// Add other pre/postconditions
	for (auto pre: getExprsFromDocTags(_node, _node.annotation(), &_node, { ASTBoogieUtils::DOCTAG_PRECOND }))
	{
		procDecl->getRequires().push_back(bg::Specification::spec(pre.expr,
							ASTBoogieUtils::createAttrs(_node.location(), "Precondition '" + pre.exprStr + "' might not hold when entering function.", *m_context.currentScanner())));
		for (auto tcc: pre.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
			procDecl->getRequires().push_back(bg::Specification::spec(tcc,
				ASTBoogieUtils::createAttrs(_node.location(), "Variables in precondition '" + pre.exprStr + "' might be out of range when entering function.", *m_context.currentScanner())));

		for (auto oc: pre.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION))
			procDecl->getRequires().push_back(bg::Specification::spec(oc,
						ASTBoogieUtils::createAttrs(_node.location(), "Overflow in computation of precondition '" + pre.exprStr + "' when entering function.", *m_context.currentScanner())));
	}
	for (auto post: getExprsFromDocTags(_node, _node.annotation(), &_node, { ASTBoogieUtils::DOCTAG_POSTCOND, ASTBoogieUtils::DOCTAG_SPECIFICATION_CASES }))
	{
		procDecl->getEnsures().push_back(bg::Specification::spec(post.expr,
							ASTBoogieUtils::createAttrs(_node.location(), "Postcondition '" + post.exprStr + "' might not hold at end of function.", *m_context.currentScanner())));
		for (auto tcc: post.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
		{
			// TCC might contain return variable, cannot be added as precondition
			tccAssumes->addStmt(bg::Stmt::assume(tcc));
			procDecl->getEnsures().push_back(bg::Specification::spec(tcc,
						ASTBoogieUtils::createAttrs(_node.location(), "Variables in postcondition '" + post.exprStr + "' might be out of range at end of function.", *m_context.currentScanner())));
		}
		for (auto oc: post.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION))
			procDecl->getEnsures().push_back(bg::Specification::spec(oc,
							ASTBoogieUtils::createAttrs(_node.location(), "Overflow in computation of postcondition '" + post.exprStr + "' at end of function.", *m_context.currentScanner())));
	}
	// TODO: check that no new sum variables were introduced

	// Add all specs for events that have been declared
	for (auto ev: m_currentEmits)
	{
		m_context.addFunctionSpecsForEvent(ev, procDecl);
	}

	// Modifies specifications
	addModifiesSpecs(_node, procDecl);

	string traceabilityName = _node.name();
	if (_node.isConstructor())
		traceabilityName = "[constructor]";
	else if (_node.isFallback())
		traceabilityName = "[fallback]";
	else if (_node.isReceive())
		traceabilityName = "[receive]";
	traceabilityName = m_context.currentContract()->name() + "::" + traceabilityName;
	procDecl->addAttrs(ASTBoogieUtils::createAttrs(_node.location(), traceabilityName, *m_context.currentScanner()));

	if (!Error::containsOnlyWarnings(errorList))
		procDecl->addAttr(bg::Attr::attr("skipped"));

	// Havoc state vars for skipped/unimplemented functions
	if (!Error::containsOnlyWarnings(errorList) || !_node.isImplemented())
	{
		for (auto contract: m_context.currentContract()->annotation().linearizedBaseContracts)
			for (auto sv: ASTNode::filteredNodes<VariableDeclaration>(contract->subNodes()))
				if (!sv->isConstant())
					procDecl->getModifies().push_back(m_context.mapDeclName(*sv));
	}

	if (!_node.isConstructor() && _node.visibility() != Visibility::External)
		m_context.addGlobalComment("\nFunction: " + _node.name() + " : " + _node.type()->toString());
	else
		m_context.addGlobalComment("\nFunction: " + _node.name());
	m_context.addDecl(procDecl);
	return false;
}

bool ASTBoogieConverter::visit(VariableDeclaration const& _node)
{
	rememberScope(_node);

	// Non-state variables should be handled in the VariableDeclarationStatement
	solAssert(_node.isStateVariable(), "Non-state variable appearing in VariableDeclaration");

	// Initializers are collected by the visitor for ContractDefinition

	// Constants are inlined
	if (_node.isConstant())
		return false;

	m_context.addGlobalComment("\nState variable: " + _node.name() + ": " + _node.type()->toString());
	// State variables are represented as maps from address to their type
	auto varDecl = bg::Decl::variable(m_context.mapDeclName(_node),
			bg::Decl::arraytype(
					m_context.addressType(),
					m_context.toBoogieType(_node.type(), &_node)));
	varDecl->addAttrs(ASTBoogieUtils::createAttrs(_node.location(), _node.name(), *m_context.currentScanner()));
	m_context.addDecl(varDecl);
	return false;
}

bool ASTBoogieConverter::visit(ModifierDefinition const& _node)
{
	rememberScope(_node);

	// Modifier definitions do not appear explicitly, but are instead inlined to functions
	return false;
}

bool ASTBoogieConverter::visit(ModifierInvocation const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: ModifierInvocation");
	return false;
}

/** Helper class to compute all state variables in an spec expression */
class ExprStateComputation : private ASTConstVisitor
{
private:

	std::set<Identifier const*>& m_stateExpressions;

public:

	/**
	 * Create a new instance with a given context.
	 */
	ExprStateComputation(std::set<Identifier const*>& modified)
	: m_stateExpressions(modified) {}

	/**	Get the base field variable of a lvale. */
	void run(Expression const& _node)
	{
		_node.accept(*this);
	}

	bool visit(Identifier const& _node) override
	{
		// Only state variables
		if (auto varDecl = dynamic_cast<VariableDeclaration const*>(_node.annotation().referencedDeclaration))
			if (varDecl && varDecl->isStateVariable())
				m_stateExpressions.insert(&_node);
		return false;
	}

};

void ASTBoogieConverter::processEventDefinition(EventDefinition const& _event)
{
	rememberScope(_event);

	// For each event emit we add:
	// - a precondition assertion: pre(saved_data)
	// - a postcondition assertion: post
	// To do so while reusing the Boogie infrastructure, we will:
	// - add a member saved_data and a flag for when it has been changed
	// - replace data in pre with saved_data
	// - declare the event as an inline function with an empty body
	// - treat both preconditions and postconditions as function preconditions in Boogie

	auto eventTracks = getExprsFromDocTags(_event, _event.annotation(), scope(), { ASTBoogieUtils::DOCTAG_EVENT_TRACKS_CHANGES });
	auto eventPreconditions = getExprsFromDocTags(_event, _event.annotation(), scope(), { ASTBoogieUtils::DOCTAG_PRECOND });
	auto eventPostconditions = getExprsFromDocTags(_event, _event.annotation(), scope(), { ASTBoogieUtils::DOCTAG_POSTCOND });

	// Are we strict about the changes when emitting
	bool strict = true;
	for (auto docTag: _event.annotation().docTags)
		if (docTag.first == "notice" && boost::starts_with(docTag.second.content, ASTBoogieUtils::DOCTAG_EVENT_ALLOW_NO_CHANGE_EMIT))
			strict = false;

	// Add all the tracked data
	for (auto e: eventTracks)
		m_context.addEventData(e.exprSol.get(), &_event);
	// Also add all variables appearing in pre- condition
	std::set<Identifier const*> stateVars;
	ExprStateComputation stateVarCompuatation(stateVars);
	for (auto pre: eventPreconditions)
		stateVarCompuatation.run(*pre.exprSol);
	for (auto post: eventPostconditions)
		stateVarCompuatation.run(*post.exprSol);
	for (auto e: stateVars)
		m_context.addEventData(e, &_event);

	// Postconditions require one more pass so that the 'before(..)' parts are correctly
	// substituted
	eventPostconditions = getExprsFromDocTags(_event, _event.annotation(), scope(), { ASTBoogieUtils::DOCTAG_POSTCOND });

	// Name of the event
	string eventName = m_context.mapDeclName(_event);

	// Input parameters
	std::vector<bg::Binding> params {
		{m_context.boogieThis()->getRefTo(), m_context.boogieThis()->getType() }, // this
		{m_context.boogieMsgSender()->getRefTo(), m_context.boogieMsgSender()->getType() }, // msg.sender
		{m_context.boogieMsgValue()->getRefTo(), m_context.boogieMsgValue()->getType() } // msg.value
	};
	for (auto par: _event.parameters())
		params.push_back({bg::Expr::id(m_context.mapDeclName(*par)), m_context.toBoogieType(par->type(), par.get())});

	// Create the procedure
	auto procDecl = m_context.declareEventProcedure(&_event, eventName, params, strict);

	// Add preconditions
	for (auto eventPre: eventPreconditions)
	{
		auto attrs = ASTBoogieUtils::createAttrs(_event.location(), "Event precondition '" + eventPre.exprStr + "' might not hold before emit of " + _event.name() + ".", *m_context.currentScanner());
		auto oldExpr = eventPre.expr->substitute(m_context.getEventDataSubstitution());
		auto spec = bg::Specification::spec(oldExpr, attrs);
		procDecl->getRequires().push_back(spec);
	}
	// Add postconditions
	for (auto eventPost: eventPostconditions)
	{
		auto attrs = ASTBoogieUtils::createAttrs(_event.location(), "Event postcondition '" + eventPost.exprStr + "' might not hold before emit " + _event.name() + ".", *m_context.currentScanner());
		auto spec = bg::Specification::spec(eventPost.expr, attrs);
		procDecl->getRequires().push_back(spec);
	}


	// Add traceability info
	string traceabilityName = _event.name();
	traceabilityName = "[event] " + m_context.currentContract()->name() + "::" + traceabilityName;
	procDecl->addAttrs(ASTBoogieUtils::createAttrs(_event.location(), traceabilityName, *m_context.currentScanner()));

	m_context.addGlobalComment("\nEvent: " + _event.name());
	m_context.addDecl(procDecl);

	// Remove the scope
	endVisitNode(_event);
}


bool ASTBoogieConverter::visit(EventDefinition const& _node)
{
	rememberScope(_node);

	return false;
}

bool ASTBoogieConverter::visit(ElementaryTypeName const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: ElementaryTypeName");
	return false;
}

bool ASTBoogieConverter::visit(UserDefinedTypeName const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: UserDefinedTypeName");
	return false;
}

bool ASTBoogieConverter::visit(FunctionTypeName const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: FunctionTypeName");
	return false;
}

bool ASTBoogieConverter::visit(Mapping const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: Mapping");
	return false;
}

bool ASTBoogieConverter::visit(ArrayTypeName const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node: ArrayTypeName");
	return false;
}

// ---------------------------------------------------------------------------
//                     Visitor methods for statements
// ---------------------------------------------------------------------------

bool ASTBoogieConverter::visit(InlineAssembly const& _node)
{
	rememberScope(_node);

	m_context.reportError(&_node, "Inline assembly is not supported");
	return false;
}

bool ASTBoogieConverter::visit(Block const& _node)
{
	rememberScope(_node);

	// Simply apply visitor recursively, compound statements will create new blocks when required
	return true;
}

bool ASTBoogieConverter::visit(PlaceholderStatement const& _node)
{
	rememberScope(_node);

	m_currentModifier++; // Go one level deeper
	processFuncModifiersAndBody();   // Process the body
	m_currentModifier--; // We are back

	return false;
}

bool ASTBoogieConverter::visit(IfStatement const& _node)
{
	rememberScope(_node);

	// Get condition recursively
	bg::Expr::Ref cond = convertExpression(_node.condition());

	// Get true branch recursively
	m_currentBlocks.push(bg::Block::block());
	_node.trueStatement().accept(*this);
	bg::Block::ConstRef thenBlock = m_currentBlocks.top();
	m_currentBlocks.pop();

	// Get false branch recursively (might not exist)
	bg::Block::ConstRef elseBlock = nullptr;
	if (_node.falseStatement())
	{
		m_currentBlocks.push(bg::Block::block());
		_node.falseStatement()->accept(*this);
		elseBlock = m_currentBlocks.top();
		m_currentBlocks.pop();
	}

	m_currentBlocks.top()->addStmt(bg::Stmt::ifelse(cond, thenBlock, elseBlock));
	return false;
}

bool ASTBoogieConverter::visit(WhileStatement const& _node)
{
	rememberScope(_node);

	string oldContinueLabel = m_currentContinueLabel;
	m_currentContinueLabel = "$continue" + util::toString(m_context.nextId());
	string oldBreakLabel = m_currentBreakLabel;
	m_currentBreakLabel = "break" + util::toString(m_context.nextId());

	// Collect invariants
	list<pair<bg::Expr::Ref, string>> invars;
	// No overflow in code
	if (m_context.overflow())
		invars.push_back({bg::Expr::not_(bg::Expr::id(ASTBoogieUtils::VERIFIER_OVERFLOW)), "No overflow"});

	std::vector<BoogieContext::DocTagExpr> loopInvars = getExprsFromDocTags(_node, _node.annotation(), scope(), { ASTBoogieUtils::DOCTAG_LOOP_INVAR });
	if (includeContractInvars(_node.annotation()))
		loopInvars.insert(end(loopInvars), begin(m_context.currentContractInvars()), end(m_context.currentContractInvars()));
	for (auto invar: loopInvars)
	{
		for (auto tcc: invar.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
			invars.push_back({tcc, "variables in range for '" + invar.exprStr + "'"});

		for (auto oc: invar.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION))
			invars.push_back({oc,"no overflow in '" + invar.exprStr + "'"});

		invars.push_back({invar.expr, invar.exprStr});
	}
	// TODO: check that invariants did not introduce new sum variables

	// Add invariants for events that have been declared
	for (auto ev: m_currentEmits)
	{
		auto invar = m_context.getEventLoopInvariant(ev);
		if (invar.first)
			invars.push_back(invar);
	}

	// Get condition recursively (create block for side effects)
	m_currentBlocks.push(bg::Block::block());
	bg::Expr::Ref cond = convertExpression(_node.condition());
	bg::Block::Ref condSideEffects = m_currentBlocks.top();
	m_currentBlocks.pop();

	// Get body recursively
	m_currentBlocks.push(bg::Block::block());
	_node.body().accept(*this);
	m_currentBlocks.top()->addStmt(bg::Stmt::label(m_currentContinueLabel));
	m_currentBlocks.top()->addStmts(condSideEffects->getStatements());
	bg::Block::Ref body = m_currentBlocks.top();
	m_currentBlocks.pop();
	m_currentContinueLabel = oldContinueLabel;

	if (_node.isDoWhile())
	{
		// Check invariants before
		for (auto inv: invars)
			m_currentBlocks.top()->addStmt(bg::Stmt::assert_(
					inv.first,
					ASTBoogieUtils::createAttrs(_node.location(), "Invariant '" + inv.second + "' might not hold on loop entry", *m_context.currentScanner())));
		// Inline body before loop
		for (auto stmt: body->getStatements())
			if (!dynamic_pointer_cast<bg::LabelStmt const>(stmt))
				m_currentBlocks.top()->addStmt(stmt);
		// Check invariants after first iteration
		for (auto inv: invars)
			m_currentBlocks.top()->addStmt(bg::Stmt::assert_(
					inv.first,
					ASTBoogieUtils::createAttrs(_node.location(), "Invariant '" + inv.second + "' might not hold after first iteration", *m_context.currentScanner())));
	}

	vector<bg::Specification::Ref> specs;
	for (auto inv: invars)
		specs.push_back(bg::Specification::spec(inv.first,
				ASTBoogieUtils::createAttrs(_node.location(), inv.second, *m_context.currentScanner())));
	m_currentBlocks.top()->addStmts(condSideEffects->getStatements());
	m_currentBlocks.top()->addStmt(bg::Stmt::while_(cond, body, specs));
	m_currentBlocks.top()->addStmt(bg::Stmt::label(m_currentBreakLabel));
	m_currentBreakLabel = oldBreakLabel;

	return false;
}

bool ASTBoogieConverter::visit(ForStatement const& _node)
{
	rememberScope(_node);

	// Boogie does not have a for statement, therefore it is transformed
	// into a while statement in the following way:
	//
	// for (initExpr; cond; loopExpr) { body }
	//
	// initExpr; while (cond) { body; loopExpr }

	string oldContinueLabel = m_currentContinueLabel;
	m_currentContinueLabel = "$continue" + util::toString(m_context.nextId());
	string oldBreakLabel = m_currentBreakLabel;
	m_currentBreakLabel = "break" + util::toString(m_context.nextId());

	// Get initialization recursively (adds statement to current block)
	m_currentBlocks.top()->addStmt(bg::Stmt::comment("The following while loop was mapped from a for loop"));
	if (_node.initializationExpression())
	{
		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Initialization"));
		_node.initializationExpression()->accept(*this);
	}

	// Get condition recursively (create block for side effects)
	m_currentBlocks.push(bg::Block::block());
	bg::Expr::Ref cond = _node.condition() ? convertExpression(*_node.condition()) : nullptr;
	bg::Block::Ref condSideEffects = m_currentBlocks.top();
	m_currentBlocks.pop();
	m_currentBlocks.top()->addStmts(condSideEffects->getStatements());

	// Get body recursively
	m_currentBlocks.push(bg::Block::block());
	m_currentBlocks.top()->addStmt(bg::Stmt::comment("Body"));
	_node.body().accept(*this);
	m_currentBlocks.top()->addStmt(bg::Stmt::label(m_currentContinueLabel));
	// Include loop expression at the end of body
	if (_node.loopExpression())
	{
		m_currentBlocks.top()->addStmt(bg::Stmt::comment("Loop expression"));
		_node.loopExpression()->accept(*this); // Adds statements to current block
	}
	m_currentBlocks.top()->addStmts(condSideEffects->getStatements());
	bg::Block::ConstRef body = m_currentBlocks.top();
	m_currentBlocks.pop();
	m_currentContinueLabel = oldContinueLabel;

	std::vector<bg::Specification::Ref> invars;

	// No overflow in code
	if (m_context.overflow())
	{
		invars.push_back(bg::Specification::spec(
				bg::Expr::not_(bg::Expr::id(ASTBoogieUtils::VERIFIER_OVERFLOW)),
				ASTBoogieUtils::createAttrs(_node.location(), "No overflow", *m_context.currentScanner())
		));
	}

	std::vector<BoogieContext::DocTagExpr> loopInvars = getExprsFromDocTags(_node, _node.annotation(), &_node, { ASTBoogieUtils::DOCTAG_LOOP_INVAR });
	if (includeContractInvars(_node.annotation()))
		loopInvars.insert(end(loopInvars), begin(m_context.currentContractInvars()), end(m_context.currentContractInvars()));
	for (auto invar: loopInvars)
	{
		for (auto tcc: invar.conditions.getConditions(ExprConditionStore::ConditionType::TYPE_CHECKING_CONDITION))
			invars.push_back(bg::Specification::spec(tcc,
					ASTBoogieUtils::createAttrs(_node.location(), "variables in range for '" + invar.exprStr + "'", *m_context.currentScanner())));

		for (auto oc: invar.conditions.getConditions(ExprConditionStore::ConditionType::OVERFLOW_CONDITION))
			invars.push_back(bg::Specification::spec(oc,
					ASTBoogieUtils::createAttrs(_node.location(), "no overflow in '" + invar.exprStr + "'", *m_context.currentScanner())));

		invars.push_back(bg::Specification::spec(invar.expr, ASTBoogieUtils::createAttrs(_node.location(), invar.exprStr, *m_context.currentScanner())));
	}
	// TODO: check that invariants did not introduce new sum variables

	// Add invariants for events that have been declared
	for (auto ev: m_currentEmits)
	{
		auto invar = m_context.getEventLoopInvariant(ev);
		if (invar.first)
		{
			auto invarAttrs = ASTBoogieUtils::createAttrs(_node.location(), invar.second, *m_context.currentScanner());
			invars.push_back(bg::Specification::spec(invar.first, invarAttrs));
		}
	}

	m_currentBlocks.top()->addStmt(bg::Stmt::while_(cond, body, invars));
	m_currentBlocks.top()->addStmt(bg::Stmt::label(m_currentBreakLabel));
	m_currentBreakLabel = oldBreakLabel;

	return false;
}

bool ASTBoogieConverter::visit(Continue const& _node)
{
	rememberScope(_node);
	m_currentBlocks.top()->addStmt(bg::Stmt::goto_({m_currentContinueLabel}));
	return false;
}

bool ASTBoogieConverter::visit(Break const& _node)
{
	rememberScope(_node);
	m_currentBlocks.top()->addStmt(bg::Stmt::goto_({m_currentBreakLabel}));
	return false;
}

bool ASTBoogieConverter::visit(Return const& _node)
{
	rememberScope(_node);

	if (_node.expression() != nullptr)
	{
		// Get rhs recursively
		bg::Expr::Ref rhs = convertExpression(*_node.expression());

		// Return type
		TypePointer returnType = nullptr;
		auto const& returnParams = m_currentFunc->returnParameters();
		if (returnParams.size() > 1)
		{
			std::vector<TypePointer> elems;
			for (auto p: returnParams)
				elems.push_back(p->annotation().type);
			returnType = TypeProvider::tuple(elems);
		}
		else
			returnType = returnParams[0]->annotation().type;

		auto rhsType = _node.expression()->annotation().type;

		// LHS of assignment should already be known (set by the enclosing FunctionDefinition)
		bg::Expr::Ref lhs = m_currentRet;

		// First create an assignment, and then an empty return
		auto ar = AssignHelper::makeAssign(
				AssignHelper::AssignParam{lhs, returnType, nullptr},
				AssignHelper::AssignParam{rhs, rhsType, _node.expression()},
						Token::Assign, &_node, m_context);
		m_localDecls.insert(m_localDecls.end(), ar.newDecls.begin(), ar.newDecls.end());
		for (auto stmt: ar.newStmts)
			m_currentBlocks.top()->addStmt(stmt);
	}
	m_currentBlocks.top()->addStmt(bg::Stmt::goto_({m_currentReturnLabel}));
	return false;
}

bool ASTBoogieConverter::visit(Throw const& _node)
{
	rememberScope(_node);

	m_currentBlocks.top()->addStmt(bg::Stmt::assume(bg::Expr::false_()));
	return false;
}

bool ASTBoogieConverter::visit(EmitStatement const& _node)
{
	rememberScope(_node);

	// Call the event "function"
	FunctionCall const& eventCall = _node.eventCall();
	convertExpression(eventCall);

	return false;
}

bool ASTBoogieConverter::visit(VariableDeclarationStatement const& _node)
{
	rememberScope(_node);

	auto const& declarations = _node.declarations();
	auto initialValue = _node.initialValue();

	if (declarations.size() == 1)
	{
		bool localPtr = false;
		if (auto refType = dynamic_cast<ReferenceType const*>(declarations[0]->type()))
		{
			if (refType->dataStoredIn(DataLocation::Storage) && refType->isPointer())
				localPtr = true;
		}
		else
			localPtr = dynamic_cast<MappingType const*>(declarations[0]->type());

		if (localPtr)
		{
			solAssert(initialValue, "Uninitialized local storage pointer.");
			bg::Expr::Ref init = convertExpression(*initialValue);

			auto packed = StoragePtrHelper::packToLocalPtr(initialValue, init, m_context);
			auto varDecl = bg::Decl::variable(
					m_context.mapDeclName(*declarations[0]),
					m_context.localPtrType());
			m_localDecls.push_back(varDecl);
			m_currentBlocks.top()->addStmt(bg::Stmt::assign(varDecl->getRefTo(), packed));
			return false;
		}
	}

	for (auto decl: declarations)
	{
		// Decl can be null, e.g., var (x,,) = (1,2,3)
		// In this case we just ignore it
		if (decl != nullptr)
		{
			solAssert(decl->isLocalVariable(), "Non-local variable appearing in VariableDeclarationStatement");
			// Boogie requires local variables to be declared at the beginning of the procedure
			auto varDecl = bg::Decl::variable(
					m_context.mapDeclName(*decl),
					m_context.toBoogieType(decl->type(), decl.get()));
			varDecl->addAttrs(ASTBoogieUtils::createAttrs(decl->location(), decl->name(), *m_context.currentScanner()));
			m_localDecls.push_back(varDecl);
		}
	}

	// Convert initial value into an assignment statement
	if (initialValue)
	{
		auto initalValueType = initialValue->annotation().type;

		// Get expression recursively
		bg::Expr::Ref rhs = convertExpression(*initialValue);

		if (declarations.size() == 1)
		{
			// One return value, simple
			auto decl = declarations[0];
			auto declType = decl->type();

			auto ar = AssignHelper::makeAssign(
					AssignHelper::AssignParam{bg::Expr::id(m_context.mapDeclName(*decl)), declType, nullptr},
					AssignHelper::AssignParam{rhs, initalValueType, initialValue},
					Token::Assign, &_node, m_context);
			m_localDecls.insert(m_localDecls.end(), ar.newDecls.begin(), ar.newDecls.end());
			for (auto stmt: ar.newStmts)
				m_currentBlocks.top()->addStmt(stmt);

		}
		else
		{
			auto initTupleType = dynamic_cast<TupleType const*>(initalValueType);
			auto initTuple = dynamic_cast<TupleExpression const*>(initialValue);
			auto rhsTuple = dynamic_pointer_cast<bg::TupleExpr const>(rhs);
			if (rhsTuple == nullptr)
			{
				m_context.reportError(initialValue, "Initialization of tuples with non-tuples is not supported.");
				return false;
			}

			for (size_t i = 0; i < declarations.size(); ++ i)
			{
				auto decl = declarations[i];
				if (decl != nullptr)
				{
					auto declType = decl->type();
					auto exprType = initTupleType->components()[i];
					auto rhs_i = rhsTuple->elements()[i];

					auto ar = AssignHelper::makeAssign(
							AssignHelper::AssignParam{bg::Expr::id(m_context.mapDeclName(*decl)), declType, nullptr},
							AssignHelper::AssignParam{rhs_i, exprType, initTuple ? initTuple->components().at(i).get() : nullptr},
									Token::Assign, &_node, m_context);
					m_localDecls.insert(m_localDecls.end(), ar.newDecls.begin(), ar.newDecls.end());
					for (auto stmt: ar.newStmts)
						m_currentBlocks.top()->addStmt(stmt);
				}
			}
		}
	}
	// Otherwise initialize with default value
	else
	{
		for (auto declNode: _node.declarations())
		{
			// Try default value
			bg::Expr::Ref defaultVal = ASTBoogieUtils::defaultValue(declNode->type(), m_context);
			if (defaultVal)
			{
				m_currentBlocks.top()->addStmt(bg::Stmt::assign(
									bg::Expr::id(m_context.mapDeclName(*declNode)), defaultVal));
			}
			else
			{
				// Default value for memory arrays
				auto arrType = dynamic_cast<ArrayType const*>(declNode->type());
				auto structType = dynamic_cast<StructType const*>(declNode->type());
				if (arrType && arrType->location() == DataLocation::Memory)
				{
					auto result = ASTBoogieUtils::newArray(m_context.toBoogieType(declNode->type(), declNode.get()), m_context);
					auto varDecl = result.newDecl;
					for (auto stmt: result.newStmts)
						m_currentBlocks.top()->addStmt(stmt);
					m_localDecls.push_back(varDecl);
					m_currentBlocks.top()->addStmt(bg::Stmt::assign(
							bg::Expr::id(m_context.mapDeclName(*declNode)), varDecl->getRefTo()));
					auto bgType = m_context.toBoogieType(arrType->baseType(), declNode.get());
					auto memArr = m_context.getMemArray(varDecl->getRefTo(), bgType);
					auto arrLen = m_context.getArrayLength(memArr, bgType);
					m_currentBlocks.top()->addStmt(bg::Stmt::assign(arrLen, m_context.intLit(0, 256)));
				}
				else if (structType && structType->location() == DataLocation::Memory)
				{
					auto structDef = &structType->structDefinition();
					auto result = ASTBoogieUtils::newStruct(structDef, m_context);
					auto varDecl = result.newDecl;
					for (auto stmt: result.newStmts)
						m_currentBlocks.top()->addStmt(stmt);
					m_localDecls.push_back(varDecl);
					m_currentBlocks.top()->addStmt(bg::Stmt::assign(
							bg::Expr::id(m_context.mapDeclName(*declNode)), varDecl->getRefTo()));
					for (size_t i = 0; i < structDef->members().size(); ++i)
					{
						auto member = bg::Expr::id(m_context.mapDeclName(*structDef->members()[i]));
						auto defVal = ASTBoogieUtils::defaultValue(structDef->members()[i]->type(), m_context);
						if (defVal)
							m_currentBlocks.top()->addStmt(bg::Stmt::assign(member, bg::Expr::arrupd(member, varDecl->getRefTo(), defVal)));
						else
							m_context.reportWarning(declNode.get(), "Unhandled default value, verification might fail");
					}
				}
				else
					m_context.reportWarning(declNode.get(), "Unhandled default value, verification might fail");
			}
		}
	}
	return false;
}

bool ASTBoogieConverter::visit(ExpressionStatement const& _node)
{
	rememberScope(_node);
	convertExpression(_node.expression());
	return false;
}


bool ASTBoogieConverter::visitNode(ASTNode const& _node)
{
	rememberScope(_node);

	solAssert(false, "Unhandled node (unknown)");
	return true;
}

}
}
