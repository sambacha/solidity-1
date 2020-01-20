#pragma once
#include <libsolidity/ast/Types.h>
#include <libsolidity/boogie/BoogieAst.h>

namespace dev
{
namespace solidity
{
class BoogieContext;

/**
 * Utility class for making assignments, handling conversions
 * and possible side effects.
 */
class AssignHelper
{
public:
	/** Parameter (LHS or RHS) of an assignment. */
	struct AssignParam {
		boogie::Expr::Ref bgExpr; // Boogie expression
		TypePointer type; // Type
		Expression const* expr; // Original expression (not all kinds of assignments require it)
	};

	/** Result of an assignment (including side effects). */
	struct AssignResult {
		std::list<boogie::Decl::Ref> newDecls;
		std::list<boogie::Stmt::Ref> newStmts;
		std::list<boogie::Expr::Ref> ocs;
	};

	/**
	 * Helper function to make an assignment, handling all conversions and side effects.
	 * Besides Solidity Assignment expressions, it is used for assigning return parameters,
	 * initial values, etc.
	 * @param lhs LHS subexpression
	 * @param rhs RHS subexpression
	 * @param op Operator (e.g., +=)
	 * @param assocNode Node for error reporting
	 * @param context Context
	 * @returns Assignment
	 */
	static
	AssignResult makeAssign(AssignParam lhs, AssignParam rhs, langutil::Token op, ASTNode const* assocNode, BoogieContext& context);

private:
	static
	void makeAssignInternal(AssignParam lhs, AssignParam rhs, langutil::Token op, ASTNode const* assocNode,
			BoogieContext& context, AssignResult& result);

	static
	void makeTupleAssign(AssignParam lhs, AssignParam rhs, ASTNode const* assocNode,
			BoogieContext& context, AssignResult& result);

	static
	void makeStructAssign(AssignParam lhs, AssignParam rhs, ASTNode const* assocNode,
			BoogieContext& context, AssignResult& result);

	static
	void makeArrayAssign(AssignParam lhs, AssignParam rhs, ASTNode const* assocNode,
			BoogieContext& context, AssignResult& result);

	static
	void makeBasicAssign(AssignParam lhs, AssignParam rhs, langutil::Token op, ASTNode const* assocNode,
			BoogieContext& context, AssignResult& result);

	/**
	 * Helper method to lift a conditional by pushing member accesses inside.
	 * E.g., ite(c, t, e).x becomes ite(c, t.x, e.x)
	 */
	static
	boogie::Expr::Ref liftCond(boogie::Expr::Ref expr);

	/** Checks if any sum shadow variable has to be updated based on the lhs. */
	static
	std::list<boogie::Stmt::Ref> checkForSums(boogie::Expr::Ref lhs, boogie::Expr::Ref rhs, BoogieContext& context);

	/** Helper method to deep copy a struct. */
	static
	void deepCopyStruct(StructDefinition const* structDef,
				boogie::Expr::Ref lhsBase, boogie::Expr::Ref rhsBase, DataLocation lhsLoc, DataLocation rhsLoc,
				ASTNode const* assocNode, BoogieContext& context, AssignResult& result);

};

}

}
