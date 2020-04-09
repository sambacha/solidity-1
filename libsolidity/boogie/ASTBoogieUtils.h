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
	static std::string const BOOGIE_ALLOC_COUNTER;

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

	/** Type of TCC to add */
	enum TCCType {
		/** Add only lower bound */
		LowerBound,
		/** Add only upper bound */
		UpperBound,
		/** Add both bounds */
		BothBounds
	};

	/**
	 * Depending on the context, the returned TCC can be assumed or asserted.
	 * @param expr Expression
	 * @param tp Type
	 * @returns The type checking condition for an expression with a given type.
	 */
	static
	boogie::Expr::Ref getTCCforExpr(boogie::Expr::Ref expr, TypePointer tp, TCCType type = BothBounds);

	/**
	 * @param _type Type
	 * @param context Context
	 * @returns The default value for the type
	 */
	static
	boogie::Expr::Ref defaultValue(TypePointer _type, BoogieContext& context);

	/**
	 * @param context Context
	 * @param literal String literal
	 * @returns The value corresponding to the string literal
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
	struct AllocResult
	{
		boogie::Decl::Ref newDecl;
		std::list<boogie::Stmt::Ref> newStmts;
	};

	/** Allocates a new memory struct. */
	static
	AllocResult newStruct(StructDefinition const* structDef, BoogieContext& context);

	/** Allocates a new memory array. */
	static
	AllocResult newArray(boogie::TypeDeclRef type, BoogieContext& context);
};
}
}
