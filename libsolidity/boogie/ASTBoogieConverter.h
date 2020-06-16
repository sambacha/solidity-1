#pragma once

#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/BoogieContext.h>
#include <libsolidity/parsing/Parser.h>

namespace dev
{
namespace solidity
{

/**
 * Converter from Solidity AST to Boogie AST.
 */
class ASTBoogieConverter : private ASTConstVisitor
{
private:
	BoogieContext& m_context;

	// Helper variables to pass information between the visit methods
	FunctionDefinition const* m_currentFunc; // Function currently being processed
	unsigned long m_currentModifier; // Index of the current modifier being processed

	// Collect local variable declarations (Boogie requires them at the beginning of the function).
	std::vector<boogie::Decl::Ref> m_localDecls;

	// Current block(s) where statements are appended (stack is needed due to nested blocks)
	std::stack<boogie::Block::Ref> m_currentBlocks;

	// Return statement in Solidity is mapped to an assignment to return variable(s) in Boogie
	boogie::Expr::Ref m_currentRet;
	// Current label to jump to when encountering a return. This is needed because modifiers
	// are inlined and their returns should not jump out of the whole function.
	std::string m_currentReturnLabel;
	int m_nextReturnLabelId;

	// Current labels for continue and break statements
	std::string m_currentContinueLabel;
	std::string m_currentBreakLabel;

	// Events specified by the current function and whether they are indeed emitted
	std::set<EventDefinition const*> m_currentEmits;

	/**
	 * Helper method to convert an expression using the dedicated expression converter class,
	 * it also handles side-effect statements and declarations introduced by the conversion.
	 * @param _node Solidity expression
	 * @returns Boogie expression
	 */
	boogie::Expr::Ref convertExpression(Expression const& _node);

	/**
	 * Create default constructor for a contract.
	 * @param _node Contract
	 */
	void createImplicitConstructor(ContractDefinition const& _node);

	/**
	 * Helper method to add the extra preamble that a constructor requires.
	 * Used by both regular and default constructors.
	 */
	void constructorPreamble();

	/**
	 * Create the procedure corresponding to receiving ether
	 * without a call to the fallback (selfdestruct).
	 * @param _node Contract
	 */
	void createEtherReceiveFunc(ContractDefinition const& _node);

	/**
	 * Helper method to initialize a state variable based on an explicit
	 * expression or a default value
	 * @param _node Variable to be initialized
	 */
	void initializeStateVar(VariableDeclaration const& _node);

	/**
	 * Helper method to parse an expression from a string with a given scope.
	 * @param exprStr Expression as a string
	 * @param _node Corresponding node (for error reporting)
	 * @param _scope Scope
	 * @param result Parsed expression
	 * @returns True if parsing was successful
	 */
	bool parseExpr(std::string exprStr, ASTNode const& _node, ASTNode const* _scope, BoogieContext::DocTagExpr& result);

	/**
	 * Helper method to parse a specification case expression from a string with a given scope.
	 * @param exprStr Expression as a string
	 * @param _node Corresponding node (for error reporting)
	 * @param _scope Scope
	 * @param result Parsed expression
	 * @returns True if parsing was successful
	 */
	bool parseSpecificationCasesExpr(std::string exprStr, ASTNode const& _node, ASTNode const* _scope, BoogieContext::DocTagExpr& result);

	void processSpecificationExpression(ASTPointer<Expression> specExpr, Parser::SpecificationExpressionInfo const& specInfo,
			ASTNode const& _node, ASTNode const* _scope, BoogieContext::DocTagExpr& result);

	/**
	 * Parse expressions from documentation for a given tag.
	 * @param _node Corresponding node (for error reporting)
	 * @param _annot Annotations
	 * @param _scope Scope
	 * @param _tag Tag
	 * @returns A list of parsed expressions
	 */
	std::vector<BoogieContext::DocTagExpr> getExprsFromDocTags(ASTNode const& _node, DocumentedAnnotation const& _annot,
			ASTNode const* _scope, std::vector<std::string> const& _tags);

	/**
	 * Checks if contract invariants are explicitly requested (for non-public functions).
	 * @param _annot Annotations
	 */
	bool includeContractInvars(DocumentedAnnotation const& _annot);

	/**
	 * Collect the events that the current function specifies to emit.
	 * @param _node Solidity function
	 * @returns Whether emits specs are syntactically correct
	 */
	bool collectEmitsSpecs(FunctionDefinition const& _node);

	/**
	 * Helper method to extract the variable to which the modifies specification corresponds.
	 * For example, in x[1].m, the base variable is x.
	 * @param expr Expression where the base is needed
	 * @returns The base variable
	 */
	Declaration const* getModifiesBase(Expression const* expr);

	/**
	 * Helper method to check if a base variable was reached in a modifies specification.
	 * @param expr Expression
	 * @returns True if the expression is a base variable
	 */
	bool isBaseVar(boogie::Expr::Ref expr);

	/*
	 * Helper method to replace the base variable of an expression with a value, e.g.,
	 * replacing 'x' in 'x[i].y'.
	 * @param expr Expression where replacement is needed
	 * @param value Replacement
	 * @returns Expression where the base variable was replaced
	 */
	boogie::Expr::Ref replaceBaseVar(boogie::Expr::Ref expr, boogie::Expr::Ref value);

	/**
	 * Helper method to extract and add modifies specifications to a function.
	 * @param _node Solidity function
	 * @param procDecl Corresponding Boogie procedure
	 */
	void addModifiesSpecs(FunctionDefinition const& _node, boogie::ProcDeclRef procDecl);

	/** Helper method to recursively inline modifiers and then finally process the function body. */
	void processFuncModifiersAndBody();

	/** Chronological stack of scopable nodes. */
	std::stack<ASTNode const*> m_scopes;

	/** Remember the scope of the node before visiting */
	void rememberScope(ASTNode const& _node)
	{
		if (dynamic_cast<Scopable const*>(&_node))
			m_scopes.push(&_node);
	}

	/** If the node is scopable, it will be removed from the scopes stack */
	void endVisitNode(ASTNode const& _node) override
	{
		if (m_scopes.size() > 0)
			if (m_scopes.top() == &_node)
				m_scopes.pop();
	}

	/** Returns the closest scoped node */
	ASTNode const* scope() const
	{
		if (m_scopes.size() > 0)
			return m_scopes.top();
		else
			return nullptr;
	}

	/** Process event definition */
	void processEventDefinition(EventDefinition const& event);

public:
	/**
	 * Create a new instance with a given context.
	 */
	ASTBoogieConverter(BoogieContext& context);

	/**
	 * Convert a node and add it to the actual Boogie program.
	 */
	void convert(ASTNode const& _node) { _node.accept(*this); }

	// Top-level nodes
	bool visit(SourceUnit const& _node) override;
	bool visit(PragmaDirective const& _node) override;
	bool visit(ImportDirective const& _node) override;
	bool visit(ContractDefinition const& _node) override;
	bool visit(InheritanceSpecifier const& _node) override;
	bool visit(UsingForDirective const& _node) override;
	bool visit(StructDefinition const& _node) override;
	bool visit(EnumDefinition const& _node) override;
	bool visit(EnumValue const& _node) override;
	bool visit(ParameterList const& _node) override;
	bool visit(FunctionDefinition const& _node) override;
	bool visit(VariableDeclaration const& _node) override;
	bool visit(ModifierDefinition const& _node) override;
	bool visit(ModifierInvocation const& _node) override;
	bool visit(EventDefinition const& _node) override;
	bool visit(ElementaryTypeName const& _node) override;
	bool visit(UserDefinedTypeName const& _node) override;
	bool visit(FunctionTypeName const& _node) override;
	bool visit(Mapping const& _node) override;
	bool visit(ArrayTypeName const& _node) override;

	// Statements
	bool visit(InlineAssembly const& _node) override;
	bool visit(Block const& _node) override;
	bool visit(PlaceholderStatement const& _node) override;
	bool visit(IfStatement const& _node) override;
	bool visit(WhileStatement const& _node) override;
	bool visit(ForStatement const& _node) override;
	bool visit(Continue const& _node) override;
	bool visit(Break const& _node) override;
	bool visit(Return const& _node) override;
	bool visit(Throw const& _node) override;
	bool visit(EmitStatement const& _node) override;
	bool visit(VariableDeclarationStatement const& _node) override;
	bool visit(ExpressionStatement const& _node) override;

	// Expressions are handled by a separate class

	bool visitNode(ASTNode const&) override;

};

}
}
