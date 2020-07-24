//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <libdevcore/Common.h>

namespace boogie
{
using bigint = dev::bigint;

struct Binding;

class TypeDecl;
using TypeDeclRef = std::shared_ptr<TypeDecl>;

class FuncDecl;
using FuncDeclRef = std::shared_ptr<FuncDecl>;
class DataTypeDecl;
using DataTypeDeclRef = std::shared_ptr<DataTypeDecl>;

class VarDecl;
using VarDeclRef = std::shared_ptr<VarDecl>;

class Expr
{
public:

	/** Reference to expressions */
	using Ref = std::shared_ptr<Expr const>;

	/** Comparison for references */
	struct RefCompare
	{
		bool operator() (Ref r1, Ref r2) const
		{
			return Expr::cmp(r1, r2) < 0;
		}
	};

	/** Sets of references */
	using RefSet = std::set<Ref, RefCompare>;

	/** A substitution is a map from variable names to expressions to substitute */
	using Subst = std::map<std::string, Ref>;

	virtual ~Expr() {}

	/** Print an expression in Boogie format */
	virtual void printBg(std::ostream& os) const = 0;
	std::string toBgString() const;

	/** Print an expression in SMT-LIB format */
	virtual void printSMT2(std::ostream& os) const;
	std::string toSMT2String() const;

	enum class Kind
	{
		ERROR,
		EXISTS,
		FORALL,
		AND,
		OR,
		COND,
		EQ,
		LT,
		GT,
		LTE,
		GTE,
		PLUS,
		MINUS,
		SUB,
		DIV,
		INTDIV,
		TIMES,
		MOD,
		EXP,
		FN,
		VARIABLE,
		IMPL,
		IFF,
		LIT_BOOL,
		LIT_STRING,
		LIT_INT,
		LIT_BV,
		NEQ,
		NOT,
		NEG,
		ARRAY_CONST,
		ARRAY_SELECT,
		ARRAY_UPDATE,
		DATATYPE_SELECT,
		DATATYPE_UPDATE,
		OLD,
		TUPLE,
		CONCAT
	};

	/** Get the kind of the expression */
	virtual Kind kind() const = 0;

	/** Substitutes s into the expression */
	virtual Ref substitute(Subst const& s) const = 0;

	/** Check whether the expression contains the given variable */
	virtual bool contains(std::string id) const = 0;

	bool isError() const { return kind() == Kind::ERROR; }

	/** Comparison of expressions */
	static int cmp(Ref e1, Ref e2);
	/** Lexicographic comparison of *same size* vectors of expressions */
	static int cmp(std::vector<Ref> const& l1, std::vector<Ref> const& l2);

	/** Special expression to denote errors */
	static Ref error();

	static Ref exists(std::vector<Binding> const& vars, Ref expr);
	static Ref forall(std::vector<Binding> const& vars, Ref expr);
	static Ref and_(Ref lhs, Ref rhs);
	static Ref and_(std::vector<Expr::Ref> const& exprs);
	static Ref or_(Ref lhs, Ref rhs);
	static Ref or_(std::vector<Expr::Ref> const& exprs);
	static Ref cond(Ref cond, Ref then, Ref else_);
	static Ref oneOf(std::vector<Expr::Ref> const& exprs);
	static Ref eq(Ref lhs, Ref rhs);
	static Ref lt(Ref lhs, Ref rhs);
	static Ref gt(Ref lhs, Ref rhs);
	static Ref lte(Ref lhs, Ref rhs);
	static Ref gte(Ref lhs, Ref rhs);
	static Ref plus(Ref lhs, Ref rhs);
	static Ref minus(Ref lhs, Ref rhs);
	static Ref div(Ref lhs, Ref rhs);
	static Ref intdiv(Ref lhs, Ref rhs);
	static Ref times(Ref lhs, Ref rhs);
	static Ref mod(Ref lhs, Ref rhs);
	static Ref exp(Ref lhs, Ref rhs);
	static Ref fn(std::string f, Ref x);
	static Ref fn(std::string f, Ref x, Ref y);
	static Ref fn(std::string f, Ref x, Ref y, Ref z);
	static Ref fn(std::string f, std::vector<Ref> const& args);
	static Ref id(std::string name);
	static Ref impl(Ref lhs, Ref rhs);
	static Ref iff(Ref lhs, Ref rhs);
	static Ref boollit(bool b);
	static Ref true_() { return boollit(true); }
	static Ref false_() { return boollit(false); }
	static Ref stringlit(std::string str);
	static Ref intlit(unsigned i) { return intlit((unsigned long) i); }
	static Ref intlit(unsigned long i);
	static Ref intlit(long i);
	static Ref intlit(bigint i);
	static Ref bvlit(std::string value, unsigned width);
	static Ref bvlit(bigint value, unsigned width);
	static Ref neq(Ref lhs, Ref rhs);
	static Ref not_(Ref expr);
	static Ref neg(Ref expr);
	static Ref arrconst(TypeDeclRef arrType, Ref val);
	static Ref arrsel(Ref base, Ref idx);
	static Ref arrupd(Ref base, Ref idx, Ref val);
	static Ref dtsel(Ref base, std::string mem, FuncDeclRef constr, DataTypeDeclRef dt);
	static Ref dtupd(Ref base, std::string mem, Ref val, FuncDeclRef constr, DataTypeDeclRef dt);
	static Ref old(Ref expr);
	static Ref tuple(std::vector<Ref> const& exprs);

	static Ref selectToUpdate(Ref sel, Ref value);
};

struct Binding
{
	Expr::Ref id;
	TypeDeclRef type;
};

class ErrorExpr : public Expr
{
public:
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::ERROR; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(ErrorExpr const& e) const;
};

class BinExpr : public Expr
{
public:
	enum BinaryOperator
	{
		Iff, Imp, Or, And, Eq, Neq, Lt, Gt, Lte, Gte, Sub,
		Conc, Plus, Minus, Times, Div, IntDiv, Mod, Exp
	};
private:
	BinaryOperator const op;
	Expr::Ref lhs;
	Expr::Ref rhs;
public:
	BinExpr(BinaryOperator const op, Expr::Ref lhs, Expr::Ref rhs) : op(op), lhs(lhs), rhs(rhs) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override;
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(BinExpr const& e) const;
};

class CondExpr : public Expr {
	Expr::Ref cond;
	Expr::Ref then;
	Expr::Ref else_;
public:
	CondExpr(Expr::Ref cond, Expr::Ref then, Expr::Ref else_) : cond(cond), then(then), else_(else_) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::COND; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(CondExpr const& e) const;

	Expr::Ref getCond() const { return cond; }
	Expr::Ref getThen() const { return then; }
	Expr::Ref getElse() const { return else_; }
};

class FunExpr : public Expr {
	std::string fun;
	std::vector<Ref> args;
public:
	FunExpr(std::string f, std::vector<Ref> const& args) : fun(f), args(args) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::FN; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(FunExpr const& e) const;
};

class BoolLit : public Expr {
	bool val;
public:
	BoolLit(bool b) : val(b) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::LIT_BOOL; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(BoolLit const& e) const;
};

class IntLit : public Expr {
	bigint val;
public:
	IntLit(std::string i) : val(i) {}
	IntLit(unsigned long i) : val(i) {}
	IntLit(long i) : val(i) {}
	IntLit(bigint i) : val(i) {}
	bigint getVal() const { return val; }
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::LIT_INT; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(IntLit const& e) const;
};

class BvLit : public Expr {
	std::string val;
	unsigned width;
public:
	BvLit(std::string value, unsigned width) : val(value), width(width) {}
	BvLit(bigint value, unsigned width) : width(width) {
		std::stringstream s;
		s << value;
		val = s.str();
	}
	std::string getVal() const { return val; }
	void printBg(std::ostream& os) const override;
	void printSMT2(std::ostream& os) const override;
	Kind kind() const override { return Kind::LIT_BV; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(BvLit const& e) const;
};

class StringLit : public Expr {
	std::string val;
public:
	StringLit(std::string str) : val(str) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::LIT_STRING; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(StringLit const& e) const;
};

class NegExpr : public Expr {
	Expr::Ref expr;
public:
	NegExpr(Expr::Ref expr) : expr(expr) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::NEG; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(NegExpr const& e) const;
};

class NotExpr : public Expr {
	Expr::Ref expr;
public:
	NotExpr(Expr::Ref expr) : expr(expr) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::NOT; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(NotExpr const& e) const;
};

class QuantExpr : public Expr {
public:
	enum Quantifier { Exists, Forall };
private:
	Quantifier quant;
	std::vector<Binding> vars;
	Ref expr;
public:
	QuantExpr(Quantifier q, std::vector<Binding> const& vars, Ref expr) : quant(q), vars(vars), expr(expr) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override;
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(QuantExpr const& e) const;
};

class SelExpr : public Expr {
protected:
	Ref base;
public:
	SelExpr(Ref base) : base(base) {}
	Ref getBase() const { return base; }
	virtual Ref toUpdate(Ref v) const = 0;
	virtual Ref replaceBase(Ref b) const = 0;
};

class UpdExpr : public Expr {
protected:
	Ref base;
	Ref val;
public:
	UpdExpr(Ref base, Ref val) : base(base), val(val) {}
	Ref getBase() const { return base; }
};

class ArrConstExpr : public Expr {
	TypeDeclRef arrType;
	Ref val;
public:
	ArrConstExpr(TypeDeclRef arrType, Ref val) : arrType(arrType), val(val) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::ARRAY_CONST; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(ArrConstExpr const& e) const;
};

class ArrSelExpr : public SelExpr {
	Ref idx;
public:
	ArrSelExpr(Ref base, Ref idx) : SelExpr(base), idx(idx) {}
	Ref const& getIdx() const { return idx; }
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::ARRAY_SELECT; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(ArrSelExpr const& e) const;
	Ref toUpdate(Ref value) const override { return Expr::arrupd(base, idx, value); }
	Ref replaceBase(Ref newBase) const override { return Expr::arrsel(newBase, idx); }
};

class ArrUpdExpr : public UpdExpr {
	Ref idx;
public:
	ArrUpdExpr(Ref base, Ref idx, Ref val)
		: UpdExpr(base, val), idx(idx) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::ARRAY_UPDATE; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(ArrUpdExpr const& e) const;
};

class DtSelExpr : public SelExpr {
	std::string member;
	FuncDeclRef constr;
	DataTypeDeclRef dt;
public:
	DtSelExpr(Ref base, std::string member, FuncDeclRef constr, DataTypeDeclRef dt)
		: SelExpr(base), member(member), constr(constr), dt(dt) {}
	std::string getMember() const { return member; }
	FuncDeclRef getConstr() const { return constr; }
	DataTypeDeclRef getDataType() const { return dt; }
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::DATATYPE_SELECT; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(DtSelExpr const& e) const;
	Ref toUpdate(Ref v) const override { return Expr::dtupd(base, member, v, constr, dt); }
	Ref replaceBase(Ref b) const override { return Expr::dtsel(b, member, constr, dt); }
};

class DtUpdExpr : public UpdExpr {
	std::string member;
	FuncDeclRef constr;
	DataTypeDeclRef dt;
public:
	DtUpdExpr(Ref base, std::string member, Ref v, FuncDeclRef constr, DataTypeDeclRef dt)
		: UpdExpr(base, v), member(member), constr(constr), dt(dt) {}
	Ref getBase() const { return base; }
	std::string getMember() const { return member; }
	FuncDeclRef getConstr() const { return constr; }
	DataTypeDeclRef getDataType() const { return dt; }
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::DATATYPE_UPDATE; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(DtUpdExpr const& e) const;
};

class VarExpr : public Expr {
	std::string name;
public:
	VarExpr(std::string name) : name(name) {}
	std::string getName() const { return name; }
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::VARIABLE; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(VarExpr const& e) const;
};

class OldExpr : public Expr {
	Ref expr;
public:
	OldExpr(Ref expr) : expr(expr) {}
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::OLD; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(OldExpr const& e) const;
};

class TupleExpr : public Expr {
	std::vector<Ref> elems;
public:
	TupleExpr(std::vector<Ref> const& elements): elems(elements) {}
	std::vector<Ref> const& elements() const { return elems; }
	void printBg(std::ostream& os) const override;
	Kind kind() const override { return Kind::TUPLE; }
	Ref substitute(Subst const& s) const override;
	bool contains(std::string id) const override;
	int cmp(TupleExpr const& e) const;
};

std::ostream& operator<<(std::ostream& os, Expr const& e);
std::ostream& operator<<(std::ostream& os, Expr::Ref e);

}
