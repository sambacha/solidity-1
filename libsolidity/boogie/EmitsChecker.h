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
class EmitsChecker : public ASTConstVisitor {
private:
	BoogieContext& m_context;

	std::map<CallableDeclaration const*, std::set<FunctionDefinition const*>> m_calledFuncs;
	std::map<CallableDeclaration const*, std::set<EventDefinition const*>> m_emitted;
	std::list<FunctionDefinition const*> m_allFunctions;
	CallableDeclaration const* m_currentScope;
	ContractDefinition const* m_currentContract;

public:
	EmitsChecker(BoogieContext& context);

	bool check();

	bool visit(ContractDefinition const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	bool visit(ModifierDefinition const& _node) override;
	bool visit(EmitStatement const& _node) override;
	void endVisit(ContractDefinition const& _node) override;
	void endVisit(FunctionDefinition const& _node) override;
	void endVisit(ModifierDefinition const& _node) override;

	bool visit(FunctionCall const& _node) override;

	void debug()
	{
		for (auto entry: m_calledFuncs)
		{
			std::cout << entry.first->name() << " calls:" << std::endl;
			for (auto fn: entry.second)
				std::cout << " - " << fn->name() << std::endl;
		}
	}
};

}

}
