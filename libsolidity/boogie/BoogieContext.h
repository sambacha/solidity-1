#pragma once

#include <liblangutil/ErrorReporter.h>
#include <liblangutil/EVMVersion.h>
#include <liblangutil/Scanner.h>
#include <libsolidity/ast/AST.h>
#include <libsolidity/analysis/GlobalContext.h>
#include <libsolidity/analysis/DeclarationContainer.h>
#include <libsolidity/boogie/ASTBoogieStats.h>
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/BoogieAstDecl.h>
#include <libsolidity/boogie/BoogieAstExpr.h>
#include <libsolidity/boogie/BoogieAstStmt.h>
#include <set>

namespace solidity::frontend
{

/**
 * Utility class for storing various conditions on variables/terms. Examples
 * include type-checking conditions (TCCs), overflow conditions (OCs), and
 * potentially otehrs.
 */
class ExprConditionStore
{
public:

	enum class ConditionType {
		TYPE_CHECKING_CONDITION,
		OVERFLOW_CONDITION,
	};

	typedef boogie::Expr::RefSet ConditionSet;

	/** Add a condition related to a specific variable */
	void addCondition(std::string id, ConditionType type, boogie::Expr::Ref ref);

	/** Add general condition not related to a specific variable */
	void addCondition(ConditionType type, boogie::Expr::Ref ref);

	/** Add general condition not related to specific variable */
	void addConditions(ExprConditionStore const& other);

	/** Get all conditions of this type */
	ConditionSet getConditions(ConditionType type) const;

	/** Get all conditions containing the given variable (excluding direct conditions, see getConditionsOn) */
	ConditionSet getConditionsContaining(ConditionType type, std::string id) const;

	/** Get all conditions directly involving the given variable */
	ConditionSet getConditionsOn(ConditionType type, std::string id) const;

	/** Remove all conditions containing given variable */
	void removeConditions(ConditionType type, std::string id);

	/** Remove all conditions of this type */
	void removeConditions(ConditionType type);

	/** Remove all conditions */
	void clear();

private:

	typedef std::map<ConditionType, ConditionSet> ConditionMap;

	/** All conditions not involving a particular variable */
	ConditionMap m_conditions;

	/** Conditions per variable */
	std::map<std::string, ConditionMap> m_conditionsOnVariables;
};

/**
 * Context class that is used to pass information around the different transformation classes.
 */
class BoogieContext {
public:

	/** Encoding for arithmetic types and operations. */
	enum Encoding
	{
		INT, // Use integers
		BV,  // Use bitvectors
		MOD  // Use integers with modulo operations
	};

	/** Expression (annotation) parsed from a documentation tag. */
	struct DocTagExpr {
		boogie::Expr::Ref expr; // Expression converted to Boogie
		std::string exprStr; // Expression in original format
		ASTPointer<Expression> exprSol; // Expression in Solidity AST format
		ExprConditionStore conditions; // TCCs, OCs, and similar

		DocTagExpr() {}

		DocTagExpr(boogie::Expr::Ref expr, std::string exprStr,
				ASTPointer<Expression> exprSol,
				ExprConditionStore const& conditions) :
			expr(expr), exprStr(exprStr), exprSol(exprSol), conditions(conditions) {}
	};

	/**
	 * Global context with magic variables for verification-specific functions such as sum. We
	 * use this in the name resolver, so all other stuff is already in the scope of the resolver.
	 */
	class BoogieGlobalContext : public GlobalContext
	{
		/** Next magic variable Id */
		int m_nextMagicVariableId;

		/** Ids for magic variables. */
		int newMagicVariableID();

	public:
		BoogieGlobalContext();
	};

private:
	/** Path in an array/mapping member that needs sum */
	struct SumPath {
		std::string base; // Base array over which we sum
		std::vector<std::shared_ptr<boogie::DtSelExpr const>> members; // Path of member accesses
	};

	/** Specification for an array/mapping that needs sum */
	struct SumSpec {
		SumPath path; // Path of the member we sum over
		TypePointer type; // Type of the resulting sum
		boogie::VarDeclRef shadowVar; // Shadow variable that needs to be updated
	};

	ASTBoogieStats const& m_stats;
	boogie::Program m_program; // Result of the conversion is a single Boogie program (top-level node)

	std::map<std::string, boogie::Decl::Ref> m_addressLiterals;

	boogie::VarDeclRef m_boogieBalance;
	boogie::VarDeclRef m_boogieThis;
	boogie::VarDeclRef m_boogieMsgSender;
	boogie::VarDeclRef m_boogieMsgValue;

	boogie::VarDeclRef m_boogieAllocCounter; // Counter to be increased when calling new
	std::map<StructDefinition const*,boogie::TypeDeclRef> m_memStructTypes;
	std::map<StructDefinition const*,boogie::TypeDeclRef> m_storStructTypes;
	std::map<StructDefinition const*,boogie::FuncDeclRef> m_storStructConstrs;
	// Arrays S[] for each storage struct where local pointers can point by default
	std::map<StructDefinition const*,boogie::VarDeclRef> m_defaultStorageContexts;
	// Arrays T[][] for each storage array where local pointers can point by default
	std::map<std::string,boogie::VarDeclRef> m_defaultArrStorageContexts;

	std::map<std::string,boogie::DataTypeDeclRef> m_arrDataTypes;
	std::map<std::string,boogie::FuncDeclRef> m_arrConstrs;
	std::map<std::string,boogie::TypeDeclRef> m_memArrPtrTypes;
	std::map<std::string,boogie::VarDeclRef> m_memArrs;

	std::map<std::string,boogie::FuncDeclRef> m_defaultArrays;

	Encoding m_encoding;
	bool m_overflow;
	bool m_modAnalysis;
	langutil::ErrorReporter* m_errorReporter; // Report errors with this member
	langutil::Scanner const* m_currentScanner; // Scanner used to resolve locations in the original source

	// Some members required to parse expressions from comments
	BoogieGlobalContext m_globalContext;
	std::map<ASTNode const*, std::shared_ptr<DeclarationContainer>> m_scopes;
	langutil::EVMVersion m_evmVersion;

	ContractDefinition const* m_currentContract;
	std::list<DocTagExpr> m_currentContractInvars;
	std::map<ContractDefinition const*, std::list<SumSpec>> m_currentSumSpecs;

	SourceUnit const* m_currentSource = nullptr;

	typedef std::map<std::string, boogie::Decl::ConstRef> builtin_cache;
	builtin_cache m_builtinFunctions;

	void addBuiltinFunction(boogie::FuncDeclRef fnDecl);

	bool m_transferIncluded;
	bool m_callIncluded;
	bool m_sendIncluded;

	// A stack of extra scopes (added to declaration names) used in inlining
	std::list<std::pair<ASTNode const*, std::string>> m_extraScopes;

	int m_nextId = 0;

	bool m_warnForBalances;

public:

	BoogieContext(Encoding encoding,
			bool overflow,
			bool modAnalysis,
			langutil::ErrorReporter* errorReporter,
			std::map<ASTNode const*, std::shared_ptr<DeclarationContainer>> scopes,
			langutil::EVMVersion evmVersion,
			ASTBoogieStats const& stats);

	ASTBoogieStats const& stats() const { return m_stats; }
	Encoding encoding() const { return m_encoding; }
	bool isBvEncoding() const { return m_encoding == Encoding::BV; }
	bool overflow() const { return m_overflow; }
	bool modAnalysis() const { return m_modAnalysis; }
	langutil::ErrorReporter*& errorReporter() { return m_errorReporter; }
	langutil::Scanner const*& currentScanner() { return m_currentScanner; }
	GlobalContext* globalContext() { return &m_globalContext; }
	std::map<ASTNode const*, std::shared_ptr<DeclarationContainer>>& scopes() { return m_scopes; }
	langutil::EVMVersion& evmVersion() { return m_evmVersion; }
	std::list<DocTagExpr>& currentContractInvars() { return m_currentContractInvars; }
	int nextId() { return m_nextId++; }
	boogie::VarDeclRef freshTempVar(boogie::TypeDeclRef type, std::string prefix = "tmp");
	ContractDefinition const* currentContract() const { return m_currentContract; }
	void setCurrentContract(ContractDefinition const* contract);
	SourceUnit const* currentSource() const { return m_currentSource; }
	void setCurrentSource(SourceUnit const* source) { m_currentSource = source; }
	void printErrors(std::ostream& out);

	/** Prints the Boogie program to an output stream. */
	void print(std::ostream& _stream) { m_program.print(_stream); }

	// Built-in functions and members
	void includeTransferFunction();
	void includeCallFunction();
	void includeSendFunction();

	boogie::VarDeclRef boogieBalance() const { return m_boogieBalance; }
	boogie::VarDeclRef boogieThis() const { return m_boogieThis; }
	boogie::VarDeclRef boogieMsgSender() const { return m_boogieMsgSender; }
	boogie::VarDeclRef boogieMsgValue() const { return m_boogieMsgValue; }

	/** Maps a declaration name to a name in Boogie, including extra scoping if needed. */
	std::string mapDeclName(Declaration const& decl);

	void warnForBalances();

private:

	//
	// Data related to Event tracking
	//

	using EventDataSet = std::set<Declaration const*>;

	struct EventDataInfo {
		boogie::Expr::Ref dataVar; // Data tracked
		boogie::Expr::Ref oldDataVar; // Same type, to save data
		boogie::Expr::Ref oldDataSavedVar; // Bool type, whether to update
		std::set<std::string> events; // Events tracking this field
	};

	// Information about events we're tracking
	std::map<EventDefinition const*, EventDataSet> m_eventData;
	// Set of all id's that we're tracking old values for
	std::map<Declaration const*, EventDataInfo> m_allEventData;
	// Set of solidity events that we're watching at the moment
	std::set<EventDefinition const*> m_eventDataCurrent;
	// Set of solidity events we're currently watching
	// Substitution from data members to old data members for events
	boogie::Expr::Subst m_eventDataSubstitution;

public:

	//
	// Methods related to Event tracking.
	//

	/** Returns the substitution for data tracked by events */
	boogie::Expr::Subst const& getEventDataSubstitution() const;

	/** Add new data tracked by an event */
	void addEventData(Expression const* expr, EventDefinition const* event);

	/** Enable tracking of data related to given event */
	void enableEventDataTrackingFor(EventDefinition const* event);

	/** Clear tracking of events */
	void disableEventDataTracking();

	/**
	 * Given LHS of an assignment, check if it's an update tracked by an event. Returns a statement
	 * to save the data, if not saved already.
	 */
	std::list<boogie::Stmt::Ref> checkForEventDataSave(ASTNode const* lhsExpr);

	/**
	 * Sets up the event procedure with the right pre- and post-conditions and body to capture
	 * trigger and data-update variables
	 */
	boogie::ProcDeclRef declareEventProcedure(EventDefinition const* event, std::string eventName, std::vector<boogie::Binding> const& params, bool onlyOnChanges);

	/**
	 * Adds entry and exit specs for the given function.
	 */
	void addFunctionSpecsForEvent(EventDefinition const* event, boogie::ProcDeclRef procedure);

	/** Add loop invariant for the given event. Returns null if none. */
	std::pair<boogie::Expr::Ref, std::string> getEventLoopInvariant(EventDefinition const* event) const;

	// Sum function related
private:
	/**
	 * Converts an expression to a path for summing. E.g., ss[i].x.y becomes
	 * {base: ss, members: {x, y}}
	 * Optionally a node can be given to report errors.
	 */
	void getPath(boogie::Expr::Ref expr, SumPath& path, ASTNode const* errors = nullptr);
	bool pathsEqual(SumPath const& p1, SumPath const& p2);

	boogie::Expr::Ref hashFunction(std::string name, FunctionType::Kind kind, boogie::Expr::Ref arg);

public:
	/** Gets the sum shadow variable for a given expression. If it does not exist, it is created. */
	boogie::Expr::Ref getSumVar(boogie::Expr::Ref bgExpr, Expression const* expr, TypePointer type);
	/** Initializes all sum shadow variables related to a given base declaration. */
	std::list<boogie::Stmt::Ref> initSumVars(Declaration const* decl);
	/** Updates sum variables of an assignment (lhs, rhs) if the lhs requires sum. */
	std::list<boogie::Stmt::Ref> updateSumVars(boogie::Expr::Ref lhsBg, boogie::Expr::Ref rhsBg);
	/** Havocs all sum shadow variables for the current contract */
	std::list<boogie::Stmt::Ref> havocSumVars();

	/** Push an extra scope for declarations under the scode of a given node. */
	void pushExtraScope(ASTNode const* node, std::string id) { m_extraScopes.push_back(std::make_pair(node, id)); }
	/** Pop an extra scope. */
	void popExtraScope() { m_extraScopes.pop_back(); }

	// Error reporting
	void reportError(ASTNode const* associatedNode, std::string message);
	void reportWarning(ASTNode const* associatedNode, std::string message);

	void addGlobalComment(std::string str);
	void addDecl(boogie::Decl::Ref decl);

	// Types

	/** Maps a Solidity type to a Boogie type. */
	boogie::TypeDeclRef toBoogieType(TypePointer tp, ASTNode const* _associatedNode);
	boogie::TypeDeclRef addressType() const;
	boogie::TypeDeclRef boolType() const;
	boogie::TypeDeclRef intType(unsigned size) const;
	boogie::TypeDeclRef localPtrType();
	boogie::TypeDeclRef errType() const { return boogie::Decl::elementarytype("__ERROR_UNSUPPORTED_TYPE"); }

	boogie::FuncDeclRef getStructConstructor(StructDefinition const* structDef);
	boogie::TypeDeclRef getStructType(StructDefinition const* structDef, DataLocation loc);

	/**
	 * @param type Struct type
	 * @returns Default array of strorage structs of given type where local
	 * pointers can point
	 */
	boogie::VarDeclRef getDefaultStorageContext(StructType const* type);

	/**
	 * @param type Array type
	 * @ returns Default array of storage arrays of given type where local
	 * pointers can point
	 */
	boogie::VarDeclRef getDefaultStorageContext(ArrayType const* type);

	/**
	 * Ensures that the declarations of constructor and default values are declared for arrays
	 * of the type valueType[].
	 */
	void ensureArrayDeclarations(boogie::TypeDeclRef valueType);
	boogie::FuncDeclRef getArrayConstructor(boogie::TypeDeclRef valueType);
	boogie::DataTypeDeclRef getArrayDatatype(boogie::TypeDeclRef valueType);

	/** Clean a type name so that it is a valid ID */
	static std::string cleanupTypeName(std::string typeName);

	boogie::VarDeclRef getAllocCounter() { return m_boogieAllocCounter; }
	boogie::Stmt::Ref incrAllocCounter();

	boogie::Expr::Ref getMemArray(boogie::Expr::Ref arrPtrExpr, boogie::TypeDeclRef type);
	boogie::Expr::Ref getArrayLength(boogie::Expr::Ref arrayExpr, boogie::TypeDeclRef type);
	boogie::Expr::Ref getInnerArray(boogie::Expr::Ref arrayExpr, boogie::TypeDeclRef type);

	boogie::FuncDeclRef defaultArray(boogie::TypeDeclRef keyType, boogie::TypeDeclRef valueType, std::string valueSmt);

	/** Creates an int literal corresponding to the encoding. */
	boogie::Expr::Ref intLit(boogie::bigint lit, unsigned bits) const;
	/** Gets the Boogie representation of a string literal. */
	boogie::Expr::Ref getStringLiteral(std::string str);
	/** Gets the Boogie representation of an address literal. */
	boogie::Expr::Ref getAddressLiteral(std::string addr);
	/** Slice of an integer corresponding to the encoding */
	boogie::Expr::Ref intSlice(boogie::Expr::Ref base, unsigned size, unsigned high, unsigned low);

	// Parametrized BV operations
	boogie::Expr::Ref bvExtract(boogie::Expr::Ref expr, unsigned size, unsigned high, unsigned low);
	boogie::Expr::Ref bvZeroExt(boogie::Expr::Ref expr, unsigned exprSize, unsigned resultSize);
	boogie::Expr::Ref bvSignExt(boogie::Expr::Ref expr, unsigned exprSize, unsigned resultSize);

	// Binary BV operations
	boogie::Expr::Ref bvAdd(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvSub(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvMul(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvSDiv(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvUDiv(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvAnd(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvOr(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvXor(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvAShr(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvLShr(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvShl(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvSlt(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvUlt(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvSgt(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvUgt(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvSle(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvUle(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvSge(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);
	boogie::Expr::Ref bvUge(unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs);

	// Unary BV operation
	boogie::Expr::Ref bvNeg(unsigned bits, boogie::Expr::Ref expr);
	boogie::Expr::Ref bvNot(unsigned bits, boogie::Expr::Ref expr);

	// Low lever interface
	boogie::Expr::Ref bvBinaryOp(std::string name, unsigned bits, boogie::Expr::Ref lhs, boogie::Expr::Ref rhs, boogie::TypeDeclRef resultType = nullptr);
	boogie::Expr::Ref bvUnaryOp(std::string name, unsigned bits, boogie::Expr::Ref expr);

	// Keccak function (uninterpreted)
	boogie::Expr::Ref keccak256(boogie::Expr::Ref arg);

	// SHA256 function (uninterpreted)
	boogie::Expr::Ref sha256(boogie::Expr::Ref arg);

	// RIPMD160 function (uninterpreted)
	boogie::Expr::Ref ripemd160(boogie::Expr::Ref arg);

	// Ecrecover function (uninterpreted)
	boogie::Expr::Ref ecrecover(boogie::Expr::Ref hash, boogie::Expr::Ref v, boogie::Expr::Ref r, boogie::Expr::Ref s);

};

}
