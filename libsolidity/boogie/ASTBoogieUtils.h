#pragma once

#include <libsolidity/ast/Types.h>
#include <libsolidity/boogie/BoogieAst.h>
#include <liblangutil/Scanner.h>
#include <string>

namespace dev
{
namespace solidity
{

class BoogieContext;

/**
 * Utility class for the Solidity to Boogie conversion.
 */
class ASTBoogieUtils
{
public:

	struct SolidityID {
		std::string solidity;
		std::string boogie;
	};

	// Identifiers related to the 'address' type
	static SolidityID const BALANCE;
	static SolidityID const TRANSFER;
	static SolidityID const SEND;
	static SolidityID const CALL;

	// Contract related identifiers
	static SolidityID const SUPER;
	static SolidityID const THIS;

	// Identifiers related to 'msg'
	static SolidityID const SENDER;
	static SolidityID const VALUE;

	// Other special identifiers
	static SolidityID const NOW;
	static SolidityID const BLOCKNO;

	// Error handling
	static std::string const SOLIDITY_ASSERT;
	static std::string const SOLIDITY_REQUIRE;
	static std::string const SOLIDITY_REVERT;

	// Special functions
	static std::string const SOLIDITY_KECCAK256;
	static std::string const SOLIDITY_SHA256;
	static std::string const SOLIDITY_RIPEMD160;
	static std::string const SOLIDITY_ECRECOVER;

	// Other identifiers
	static std::string const VERIFIER_SUM;
	static std::string const VERIFIER_IDX;
	static std::string const VERIFIER_OLD;
	static std::string const VERIFIER_EQ;
	static std::string const VERIFIER_OVERFLOW;
	static std::string const BOOGIE_CONSTRUCTOR;
	static std::string const BOOGIE_ZERO_ADDRESS;

	// Specification annotations
	static std::string const DOCTAG_CONTRACT_INVAR;
	static std::string const DOCTAG_LOOP_INVAR;
	static std::string const DOCTAG_CONTRACT_INVARS_INCLUDE;
	static std::string const DOCTAG_PRECOND;
	static std::string const DOCTAG_POSTCOND;
	static std::string const DOCTAG_MODIFIES;
	static std::string const DOCTAG_MODIFIES_ALL;
	static std::string const DOCTAG_MODIFIES_COND;

	/** Creates the procedure corresponding to address.transfer(). */
	static
	boogie::ProcDeclRef createTransferProc(BoogieContext& context);
	/** Creates the procedure corresponding to address.call(). */
	static
	boogie::ProcDeclRef createCallProc(BoogieContext& context);
	/** Creates the procedure corresponding to address.send().*/
	static
	boogie::ProcDeclRef createSendProc(BoogieContext& context);

	/** Print data locations as strings. */
	static
	std::string dataLocToStr(DataLocation loc);

	/**
	 * Gets the Boogie constructor name for a contract.
	 * @param contract Contract
	 * @returns Constructor name in the Boogie program
	 */
	static
	std::string getConstructorName(ContractDefinition const* contract);

	/**
	 * Creates attributes for source location with message.
	 * @param loc Source location
	 * @param message Message
	 * @param scanner Scanner to translate locations
	 * @returns Attributes
	 */
	static
	std::vector<boogie::Attr::Ref> createAttrs(langutil::SourceLocation const& loc, std::string const& message, langutil::Scanner const& scanner);

	/** An expression with the associated correctness condition. */
	struct ExprWithCC {
		boogie::Expr::Ref expr;
		boogie::Expr::Ref cc;
	};

	/**
	 * Convert a binary arithmetic operation (including ops like +=) to an expression.
	 * @param context Context
	 * @param associatedNode Node for error reporting
	 * @param op Operator
	 * @param lhs LHS subexpression
	 * @param rhs RHS subexpression
	 * @param bits Number of bits for the result
	 * @param isSigned Is the result signed
	 * @returns Expression with correctness conditions
	 */
	static
	ExprWithCC encodeArithBinaryOp(BoogieContext& context, ASTNode const* associatedNode, langutil::Token op,
			boogie::Expr::Ref lhs, boogie::Expr::Ref rhs, unsigned bits, bool isSigned);

	/**
	 * Convert a unary arithmetic operation (including ops like +=) to an expression.
	 * @param context Context
	 * @param associatedNode Node for error reporting
	 * @param op Operator
	 * @param subExpr Subexpression
	 * @param bits Number of bits for the result
	 * @param isSigned Is the result signed
	 * @returns Expression with correctness conditions
	 */
	static
	ExprWithCC encodeArithUnaryOp(BoogieContext& context, ASTNode const* associatedNode, langutil::Token op,
			boogie::Expr::Ref subExpr, unsigned bits, bool isSigned);

	/**
	 * @param type Type
	 * @returns True if the type can be represented by bitvectors
	 */
	static bool isBitPreciseType(TypePointer type);

	/**
	 * @param type Type
	 * @returns The number of bits to represent the type
	 */
	static unsigned getBits(TypePointer type);

	/**
	 * @param type Type
	 * @returns True if the type is signed
	 */
	static bool isSigned(TypePointer type);

	/**
	 * Check if implicit conversion is needed between types.
	 * For example, if exprType is uint32 and targetType is uint40,
	 * then we return an extension of 8 bits to expr.
	 * @param expr Original expression
	 * @param exprType Original type
	 * @param targetType Convert to this type
	 * @param context Context
	 * @returns The converted expression if conversion is needed, otherwise the original
	 */
	static
	boogie::Expr::Ref checkImplicitBvConversion(boogie::Expr::Ref expr, TypePointer exprType, TypePointer targetType, BoogieContext& context);

	/**
	 * Check if explicit conversion is needed between types.
	 * @param expr Original expression
	 * @param exprType Original type
	 * @param targetType Convert to this type
	 * @param context Context
	 * @returns The converted expression if conversion is needed, otherwise the original
	 */
	static
	boogie::Expr::Ref checkExplicitBvConversion(boogie::Expr::Ref expr, TypePointer exprType, TypePointer targetType, BoogieContext& context);

	/**
	 * Depending on the context, the returned TCC can be assumed or asserted.
	 * @param expr Expression
	 * @param tp Type
	 * @returns The type checking condition for an expression with a given type.
	 */
	static
	boogie::Expr::Ref getTCCforExpr(boogie::Expr::Ref expr, TypePointer tp);

	/**
	 * @param _type Type
	 * @param context Context
	 * @returns The default value for the type
	 */
	static
	boogie::Expr::Ref defaultValue(TypePointer _type, BoogieContext& context);

	/**
	 * @param const Context
	 * @return The value corresponding to the string literal
	 */
	static
	boogie::Expr::Ref stringValue(BoogieContext& context, std::string literal);

private:

	/**
	 * Helper structure for a value of a type, including complex types such as arrays
	 * and structs.
	 */
	struct Value {
		std::string smt; // Default value as SMT expression (needed for arrays)
		boogie::Expr::Ref bgExpr; // Default value as Boogie expression
	};

	static
	Value defaultValueInternal(TypePointer _type, BoogieContext& context);

public:
	/** Allocates a new memory struct. */
	static
	boogie::Decl::Ref newStruct(StructDefinition const* structDef, BoogieContext& context);

	/** Allocates a new memory array. */
	static
	boogie::Decl::Ref newArray(boogie::TypeDeclRef type, BoogieContext& context);

	// Helper methods for assignments

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

	// Local storage pointers
public:
	/* Result of packing a local storage pointer into an array. **/
	struct PackResult {
		boogie::Decl::Ref ptr; // Resulting pointer
		std::list<boogie::Stmt::Ref> stmts; // Side effects of packing (filling the pointer array)
	};

	/**
	 * Packs an expression into a local storage pointer.
	 * @param expr Expression to be packed
	 * @param bgExpr Corresponding Boogie expression
	 * @param context Context
	 * @returns Local pointer
	 */
	static
	PackResult packToLocalPtr(Expression const* expr, boogie::Expr::Ref bgExpr, BoogieContext& context);

	/**
	 * Unpacks a local storage pointer into an access to storage.
	 * @param ptrExpr Expression to be unpacked
	 * @param ptrBgExpr Corresponding Boogie expression
	 * @param context Context
	 */
	static
	boogie::Expr::Ref unpackLocalPtr(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, BoogieContext& context);

private:
	/**
	 * Packs a path to a local storage into an array. E.g., 'ss[i].t' becomes [2, i, 3] if
	 *  'ss' is the 2nd state var and 't' is the 3rd member.
	 */
	static
	void packInternal(Expression const* expr, boogie::Expr::Ref bgExpr, StructType const* structType, BoogieContext& context, PackResult& result);

	/**
	 * Unpacks an array into a tree of paths (conditional) to local storage (opposite of packing).
	 * E.g., ite(arr[0] == 0, statevar1, ite(arr[0] == 1, statevar2, ...
	 */
	static
	boogie::Expr::Ref unpackInternal(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, Declaration const* decl, int depth, boogie::Expr::Ref base, BoogieContext& context);

};

}
}
