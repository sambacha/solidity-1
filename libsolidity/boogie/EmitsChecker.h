#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/boogie/BoogieContext.h>
#include <map>
#include <set>
#include <list>

namespace dev
{
namespace solidity
{

/**
 * Checker for 'emits' specifications. All source units should be visited
 * first and then 'check' should be called.
 */
class EmitsChecker : public ASTConstVisitor {
private:
	BoogieContext& m_context;

	// Call graph
	std::map<CallableDeclaration const*, std::set<FunctionDefinition const*>> m_calledFuncs;
	// Directly emitted events
	std::map<CallableDeclaration const*, std::set<EventDefinition const*>> m_directlyEmitted;

	std::set<FunctionDefinition const*> m_allFunctions;
	CallableDeclaration const* m_currentScope;
	ContractDefinition const* m_currentContract;

	// Helper structure to store a list of overloaded events (with the same name)
	// and a flag to indicate whether any of them have been already emitted
	struct EmitsSpec {
		std::set<EventDefinition const*> events;
		bool alreadyEmitted;
	};

	/**
	 * Helper function to collect 'emits' specification for a function.
	 * @param fn Function
	 * @param specs Collect events here
	 * @returns True if there are no syntax errors, false otherwise
	 */
	bool collectEmitsSpecs(FunctionDefinition const* fn, std::list<EmitsSpec>& specs);

	bool checkIfSpecified(std::list<EmitsSpec>& specs, EventDefinition const* ev);

public:
	/**
	 * Initialize checker with a context.
	 */
	EmitsChecker(BoogieContext& context);

	/**
	 * Check if the visited source units satisfy their 'emits' specifications.
	 * @returns True if the specification is satisfied, false otherwise
	 */
	bool check();

	bool visit(ContractDefinition const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	bool visit(ModifierDefinition const& _node) override;
	bool visit(EmitStatement const& _node) override;
	void endVisit(ContractDefinition const& _node) override;
	void endVisit(FunctionDefinition const& _node) override;
	void endVisit(ModifierDefinition const& _node) override;
	bool visit(FunctionCall const& _node) override;
};

}

}
