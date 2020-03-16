#include <boost/algorithm/string/predicate.hpp>
#include <libsolidity/boogie/ASTBoogieUtils.h>
#include <libsolidity/boogie/EmitsChecker.h>

using namespace dev;
using namespace dev::solidity;
using namespace std;

bool EmitsChecker::collectEmitsSpecs(FunctionDefinition const* fn, set<EventDefinition const*>& specs)
{
	for (auto docTag: fn->annotation().docTags)
	{
		if (docTag.first == "notice" && boost::starts_with(docTag.second.content, ASTBoogieUtils::DOCTAG_EMITS))
		{
			if (auto expr = m_context.parseAnnotation(docTag.second.content.substr(ASTBoogieUtils::DOCTAG_EMITS.length() + 1), *fn, fn))
			{
				if (auto id = dynamic_pointer_cast<Identifier>(expr))
				{
					auto decl = id->annotation().referencedDeclaration;
					if (auto event = dynamic_cast<EventDefinition const*>(decl))
						specs.insert(event);
					else
					{
						m_context.reportError(fn, "Expected event in emits specification.");
						return false;
					}
				}
				else
				{
					m_context.reportError(fn, "Expected event in emits specification.");
					return false;
				}
			}
		}
	}
	return true;
}

EmitsChecker::EmitsChecker(BoogieContext& context) :
		m_context(context),
		m_currentScope(nullptr),
		m_currentContract(nullptr)
{ }

bool EmitsChecker::check()
{
	bool specsSatisfied = true;

	// First go through all functions and parse specs
	map<FunctionDefinition const*, set<EventDefinition const*>> specified;
	for (auto fn: m_allFunctions)
		if (!collectEmitsSpecs(fn, specified[fn]))
			return false;

	// Then go through functions again and check specs
	for (auto fn: m_allFunctions)
	{
		// Collect the specs for the current function with a flag
		// indicating whether the event can indeed be emitted
		map<EventDefinition const*, bool> currentFuncSpec;
		for (auto ev: specified[fn])
			currentFuncSpec[ev] = false; // Initially assume that it is not emitted

		// Check directly emitted events
		for (auto ev: m_directlyEmitted[fn])
		{
			if (currentFuncSpec.find(ev) != currentFuncSpec.end())
				currentFuncSpec[ev] = true;
			else
			{
				m_context.reportError(fn, "Function possibly emits '" + ev->name() + "' without specifying");
				specsSatisfied = false;
			}
		}

		// Check specs of called functions (incl. base constructors)
		for (auto called: m_calledFuncs[fn])
		{
			string calledName = called->isConstructor() ? "base constructor" : called->name();

			for (auto ev: specified[called])
			{
				if (currentFuncSpec.find(ev) != currentFuncSpec.end())
					currentFuncSpec[ev] = true;
				else
				{
					m_context.reportError(fn, "Function possibly emits '" + ev->name() + "' (via calling " + calledName + ") without specifying");
					specsSatisfied = false;
				}
			}
		}

		// Check what called modifiers emit
		for (auto modif: fn->modifiers())
		{
			auto modifierDecl = dynamic_cast<ModifierDefinition const*>(modif->name()->annotation().referencedDeclaration);
			if (modifierDecl)
			{
				for (auto ev: m_directlyEmitted[modifierDecl])
				{
					if (currentFuncSpec.find(ev) != currentFuncSpec.end())
						currentFuncSpec[ev] = true;
					else
					{
						m_context.reportError(fn, "Function possibly emits '" + ev->name() +
								"' (via modifier " + modifierDecl->name() + ") without specifying");
						specsSatisfied = false;
					}
				}
			}
			// TODO: else report unsupported case (except if it is a base constructor call)
		}

		// Finally give warnings for specified but not emitted events
		for (auto entry: currentFuncSpec)
		{
			if (!entry.second)
				m_context.reportWarning(fn, "Function specifies '" + entry.first->name() + "' but never emits.");
		}
	}

	return specsSatisfied;
}

bool EmitsChecker::visit(ContractDefinition const& _node)
{
	m_currentContract = &_node;
	return true;
}

bool EmitsChecker::visit(FunctionDefinition const& _node)
{
	m_currentScope = &_node;
	m_allFunctions.push_back(&_node);

	// Add base constructor calls
	solAssert(m_currentContract, "FunctionDefinition without ContractDefinition");
	if (_node.isConstructor())
		for (auto base: m_currentContract->annotation().linearizedBaseContracts)
			for (auto fn: ASTNode::filteredNodes<FunctionDefinition>(base->subNodes()))
				if (fn->isConstructor())
					m_calledFuncs[m_currentScope].insert(fn);

	return true;
}

bool EmitsChecker::visit(ModifierDefinition const& _node)
{
	m_currentScope = &_node;
	return true;
}

bool EmitsChecker::visit(EmitStatement const& _node)
{
	solAssert(m_currentScope, "EmitStatement without scope");
	EventDefinition const* eventDef = nullptr;
	if (auto id = dynamic_cast<Identifier const*>(&_node.eventCall().expression()))
		eventDef = dynamic_cast<EventDefinition const*>(id->annotation().referencedDeclaration);

	if (!eventDef)
	{
		m_context.reportError(&_node, "Unsupported emit statement, could not determine event");
		return false;
	}
	m_directlyEmitted[m_currentScope].insert(eventDef);
	return true;
}

void EmitsChecker::endVisit(ContractDefinition const&)
{
	m_currentContract = nullptr;
}

void EmitsChecker::endVisit(FunctionDefinition const&)
{
	m_currentScope = nullptr;
}

void EmitsChecker::endVisit(ModifierDefinition const&)
{
	m_currentScope = nullptr;
}

bool EmitsChecker::visit(FunctionCall const& _node)
{
	// We are not in any scope, this should not be an 'actual' function call
	// that we are interested in. E.g., conversion in state variable initializer
	if (!m_currentScope)
		return false;

	FunctionDefinition const* calledDef = nullptr;
	if (auto id = dynamic_cast<Identifier const*>(&_node.expression()))
		calledDef = dynamic_cast<FunctionDefinition const*>(id->annotation().referencedDeclaration);
	if (auto memAcc = dynamic_cast<MemberAccess const*>(&_node.expression()))
		calledDef = dynamic_cast<FunctionDefinition const*>(memAcc->annotation().referencedDeclaration);

	// TODO: handle other cases for function calls
	// TODO: if we cannot get the definition we don't give a warning yet because it would break a lot of tests

	if (calledDef)
		m_calledFuncs[m_currentScope].insert(calledDef);

	return true;
}
