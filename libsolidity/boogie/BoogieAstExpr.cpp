//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/BoogieAstDecl.h>
#include <libsolidity/boogie/BoogieAstExpr.h>
#include <liblangutil/Exceptions.h>
#include <sstream>
#include <iostream>

namespace boogie {

void Expr::printSMT2(std::ostream& out) const
{
	printBg(out); // By default print Boogie, override if differs
}

std::string Expr::toBgString() const
{
	std::stringstream ss;
	printBg(ss);
	return ss.str();
}

std::string Expr::toSMT2String() const
{
	std::stringstream ss;
	printSMT2(ss);
	return ss.str();
}

Expr::Ref Expr::error()
{
	return std::make_shared<ErrorExpr const>();
}

Expr::Ref Expr::exists(std::vector<Binding> const& vars, Ref expr)
{
	return std::make_shared<QuantExpr const>(QuantExpr::Exists, vars, expr);
}

Expr::Ref Expr::forall(std::vector<Binding> const& vars, Ref expr)
{
	return std::make_shared<QuantExpr const>(QuantExpr::Forall, vars, expr);
}

Expr::Ref Expr::and_(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::And, lhs, rhs);
}

Expr::Ref Expr::and_(std::vector<Expr::Ref> const& exprs)
{
	if (exprs.size() == 0)
		return true_();
	Expr::Ref result = exprs[0];
	for (size_t i = 1; i < exprs.size(); ++ i)
		result = and_(result, exprs[i]);
	return result;
}

Expr::Ref Expr::or_(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Or, lhs, rhs);
}

Expr::Ref Expr::or_(std::vector<Expr::Ref> const& exprs)
{
	if (exprs.size() == 0)
		return false_();
	Expr::Ref result = exprs[0];
	for (size_t i = 1; i < exprs.size(); ++ i)
		result = or_(result, exprs[i]);
	return result;
}

Expr::Ref Expr::cond(Ref cond, Ref then, Ref else_)
{
	return std::make_shared<CondExpr const>(cond, then, else_);
}

Expr::Ref Expr::oneOf(std::vector<Expr::Ref> const& exprs)
{
	std::vector<Expr::Ref> conjuncts(exprs);
	std::vector<Expr::Ref> disjuncts;
	for (size_t i = 0; i < exprs.size(); ++ i) {
		Expr::Ref pick = exprs[i];
		conjuncts[i] = not_(pick);
		disjuncts.push_back(and_(conjuncts));
		conjuncts[i] = pick;
	}
	return or_(disjuncts);
}


Expr::Ref Expr::eq(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Eq, lhs, rhs);
}

Expr::Ref Expr::lt(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Lt, lhs, rhs);
}

Expr::Ref Expr::gt(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Gt, lhs, rhs);
}

Expr::Ref Expr::lte(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Lte, lhs, rhs);
}

Expr::Ref Expr::gte(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Gte, lhs, rhs);
}

Expr::Ref Expr::plus(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Plus, lhs, rhs);
}

Expr::Ref Expr::minus(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Minus, lhs, rhs);
}

Expr::Ref Expr::div(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Div, lhs, rhs);
}

Expr::Ref Expr::intdiv(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::IntDiv, lhs, rhs);
}

Expr::Ref Expr::times(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Times, lhs, rhs);
}

Expr::Ref Expr::mod(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Mod, lhs, rhs);
}

Expr::Ref Expr::exp(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Exp, lhs, rhs);
}

Expr::Ref Expr::fn(std::string f, std::vector<Ref> const& args)
{
	return std::make_shared<FunExpr const>(f, args);
}

Expr::Ref Expr::fn(std::string f, Ref x)
{
	return std::make_shared<FunExpr const>(f, std::vector<Ref>{x});
}

Expr::Ref Expr::fn(std::string f, Ref x, Ref y)
{
	return std::make_shared<FunExpr const>(f, std::vector<Ref>{x, y});
}

Expr::Ref Expr::fn(std::string f, Ref x, Ref y, Ref z)
{
	return std::make_shared<FunExpr const>(f, std::vector<Ref>{x, y, z});
}

Expr::Ref Expr::id(std::string name)
{
	return std::make_shared<VarExpr const>(name);
}

Expr::Ref Expr::impl(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Imp, lhs, rhs);
}

Expr::Ref Expr::iff(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Iff, lhs, rhs);
}

Expr::Ref Expr::boollit(bool b)
{
	return std::make_shared<BoolLit const>(b);
}

Expr::Ref Expr::stringlit(std::string str)
{
	return std::make_shared<StringLit const>(str);
}

Expr::Ref Expr::intlit(unsigned long i)
{
	return std::make_shared<IntLit const>(i);
}

Expr::Ref Expr::intlit(long i)
{
	return std::make_shared<IntLit const>(i);
}

Expr::Ref Expr::intlit(bigint i)
{
	return std::make_shared<IntLit const>(i);
}

Expr::Ref Expr::bvlit(std::string value, unsigned width)
{
	return std::make_shared<BvLit const>(value, width);
}

Expr::Ref Expr::bvlit(bigint value, unsigned width)
{
	return std::make_shared<BvLit const>(value, width);
}

Expr::Ref Expr::neq(Ref lhs, Ref rhs)
{
	return std::make_shared<BinExpr const>(BinExpr::Neq, lhs, rhs);
}

Expr::Ref Expr::not_(Ref expr)
{
	return std::make_shared<NotExpr const>(expr);
}

Expr::Ref Expr::neg(Ref expr)
{
	return std::make_shared<NegExpr const>(expr);
}

Expr::Ref Expr::arrconst(TypeDeclRef arrType, Ref val)
{
	return std::make_shared<ArrConstExpr const>(arrType, val);
}

Expr::Ref Expr::arrsel(Ref base, Ref idx)
{
	return std::make_shared<ArrSelExpr const>(base, idx);
}

Expr::Ref Expr::arrupd(Ref base, Ref idx, Ref val)
{
	return std::make_shared<ArrUpdExpr const>(base, idx, val);
}

Expr::Ref Expr::dtsel(Ref base, std::string mem, FuncDeclRef constr, DataTypeDeclRef dt)
{
	return std::make_shared<DtSelExpr>(base, mem, constr, dt);
}

Expr::Ref Expr::dtupd(Ref base, std::string mem, Ref val, FuncDeclRef constr, DataTypeDeclRef dt)
{
	return std::make_shared<DtUpdExpr>(base, mem, val, constr, dt);
}

Expr::Ref Expr::old(Ref expr)
{
	return std::make_shared<OldExpr const>(expr);
}

Expr::Ref Expr::tuple(std::vector<Ref> const& elems)
{
	return std::make_shared<TupleExpr const>(elems);
}

Expr::Ref Expr::selectToUpdate(Expr::Ref sel, Expr::Ref value)
{
	if (auto selExpr = std::dynamic_pointer_cast<SelExpr const>(sel))
	{
		if (auto base = std::dynamic_pointer_cast<SelExpr const>(selExpr->getBase()))
			return selectToUpdate(base, selExpr->toUpdate(value));
		else
			return selExpr->toUpdate(value);
	}
	solAssert(false, "Expected datatype/array select");
	return nullptr;
}

std::ostream& operator<<(std::ostream& os, Expr const& e)
{
	e.printBg(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Expr::Ref e)
{
	e->printBg(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Binding const& p)
{
	os << p.id << ": " << p.type->getName();
	return os;
}

void ErrorExpr::printBg(std::ostream& os) const
{
	os << "ERROR";
}

void BinExpr::printBg(std::ostream& os) const
{
	os << "(" << lhs << " ";
	switch (op)
	{
	case Iff:
		os << "<==>";
		break;
	case Imp:
		os << "==>";
		break;
	case Or:
		os << "||";
		break;
	case And:
		os << "&&";
		break;
	case Eq:
		os << "==";
		break;
	case Neq:
		os << "!=";
		break;
	case Lt:
		os << "<";
		break;
	case Gt:
		os << ">";
		break;
	case Lte:
		os << "<=";
		break;
	case Gte:
		os << ">=";
		break;
	case Sub:
		os << "<:";
		break;
	case Conc:
		os << "++";
		break;
	case Plus:
		os << "+";
		break;
	case Minus:
		os << "-";
		break;
	case Times:
		os << "*";
		break;
	case Div:
		os << "/";
		break;
	case IntDiv:
		os << "div";
		break;
	case Mod:
		os << "mod";
		break;
	case Exp:
		os << "**";
		break;
	}
	os << " " << rhs << ")";
}

void CondExpr::printBg(std::ostream& os) const
{
	os << "(if " << cond << " then " << then << " else " << else_ << ")";
}

void FunExpr::printBg(std::ostream& os) const
{
	os << fun;
	print_seq(os, args, "(", ", ", ")");
}

void BoolLit::printBg(std::ostream& os) const
{
	os << (val ? "true" : "false");
}

void IntLit::printBg(std::ostream& os) const
{
	os << val;
}

void BvLit::printBg(std::ostream& os) const
{
	os << val << "bv" << width;
}

void BvLit::printSMT2(std::ostream& os) const
{
	os << "(_ bv" << val << " " << width << ")";
}

void NegExpr::printBg(std::ostream& os) const
{
	os << "-(" << expr << ")";
}

void NotExpr::printBg(std::ostream& os) const
{
	os << "!(" << expr << ")";
}

void QuantExpr::printBg(std::ostream& os) const
{
	os << "(";
	switch (quant)
	{
	case Forall:
		os << "forall ";
		break;
	case Exists:
		os << "exists ";
		break;
	}
	print_seq(os, vars, ", ");
	os << " :: " << expr << ")";
}

void ArrConstExpr::printBg(std::ostream& os) const
{
	os << "((as const " << arrType << ") " << val << ")";
}

void ArrSelExpr::printBg(std::ostream& os) const
{
	os << base << "[" << idx << "]";
}

void ArrUpdExpr::printBg(std::ostream& os) const
{
	os << base << "[" << idx << " := " << val << "]";
}

void VarExpr::printBg(std::ostream& os) const
{
	os << name;
}

void OldExpr::printBg(std::ostream& os) const
{
	os << "old(";
	expr->printBg(os);
	os << ")";
}

void TupleExpr::printBg(std::ostream& os) const
{
	print_seq(os, elems, ", ");
}

void StringLit::printBg(std::ostream& os) const
{
	os << "\"" << val << "\"";
}

void DtSelExpr::printBg(std::ostream& os) const
{
	os << member << "#" << constr->getName();
	os << "(";
	base->printBg(os);
	os << ")";
}

void DtUpdExpr::printBg(std::ostream& os) const
{
	os << constr->getName();
	os << "(";
	for (size_t i = 0; i < dt->getMembers().size(); i++)
	{
		auto mem = dt->getMembers()[i];
		std::string memberName = std::dynamic_pointer_cast<VarExpr const>(mem.id)->getName();
		if (memberName == member)
			val->printBg(os);
		else
			Expr::dtsel(base, memberName, constr, dt)->printBg(os);

		if (i < dt->getMembers().size() - 1)
			os << ", ";
	}
	os << ")";
}

//
// Substitution stuff
//

Expr::Ref ErrorExpr::substitute(Expr::Subst const& s) const
{
	(void)s;
	return Expr::error();
}

Expr::Ref BinExpr::substitute(Expr::Subst const& s) const
{
	Ref lhs1 = lhs->substitute(s);
	Ref rhs1 = rhs->substitute(s);
	return std::make_shared<BinExpr const>(op, lhs1, rhs1);
}

Expr::Ref CondExpr::substitute(Expr::Subst const& s) const
{
	Ref cond1 = cond->substitute(s);
	Ref then1 = then->substitute(s);
	Ref else1 = else_->substitute(s);
	return std::make_shared<CondExpr const>(cond1, then1, else1);
}

Expr::Ref FunExpr::substitute(Expr::Subst const& s) const
{
	std::vector<Ref> args1;
	for (Ref a: args)
		args1.push_back(a->substitute(s));
	return std::make_shared<FunExpr const>(fun, args1);
}

Expr::Ref BoolLit::substitute(Expr::Subst const& s) const
{
	(void)s;
	return std::make_shared<BoolLit const>(val);
}

Expr::Ref IntLit::substitute(Expr::Subst const& s) const
{
	(void)s;
	return std::make_shared<IntLit const>(val);
}

Expr::Ref BvLit::substitute(Expr::Subst const& s) const
{
	(void)s;
	return std::make_shared<BvLit const>(val, width);
}

Expr::Ref NegExpr::substitute(Expr::Subst const& s) const
{
	Ref expr1 = expr->substitute(s);
	return std::make_shared<NegExpr const>(expr1);
}

Expr::Ref NotExpr::substitute(Expr::Subst const& s) const
{
	Ref expr1 = expr->substitute(s);
	return std::make_shared<NotExpr const>(expr1);
}

Expr::Ref QuantExpr::substitute(Expr::Subst const& s) const
{
	Expr::Subst s1(s);

	// Setup the binding (remove any bound variables from substitution)
	for (Binding b: vars)
	{
		auto varExpr = std::dynamic_pointer_cast<VarExpr const>(b.id);
		solAssert(varExpr, "Binding is not a variable");
		std::string id = varExpr->getName();
		auto idFind = s.find(id);
		if (idFind != s.end())
			s1.erase(idFind);
	}

	// Substitute the expression
	Ref expr1 = expr->substitute(s1);
	return std::make_shared<QuantExpr const>(quant, vars, expr1);
}

Expr::Ref ArrConstExpr::substitute(Expr::Subst const& s) const
{
	Ref val1 = val->substitute(s);
	return std::make_shared<ArrConstExpr const>(arrType, val1);
}

Expr::Ref ArrSelExpr::substitute(Expr::Subst const& s) const
{
	Ref base1 = base->substitute(s);
	Ref idx1 = idx->substitute(s);
	return std::make_shared<ArrSelExpr const>(base1, idx1);
}

Expr::Ref ArrUpdExpr::substitute(Expr::Subst const& s) const
{
	Ref base1 = base->substitute(s);
	Ref idx1 = idx->substitute(s);
	Ref val1 = val->substitute(s);
	return std::make_shared<ArrUpdExpr const>(base1, idx1, val1);
}

Expr::Ref VarExpr::substitute(Expr::Subst const& s) const
{
	auto find = s.find(name);
	if (find != s.end())
		return find->second;
	else
		return std::make_shared<VarExpr const>(name);
}

Expr::Ref OldExpr::substitute(Expr::Subst const& s) const
{
	Ref expr1 = expr->substitute(s);
	return std::make_shared<OldExpr const>(expr1);
}

Expr::Ref TupleExpr::substitute(Expr::Subst const& s) const
{
	std::vector<Ref> es1;
	for (Ref e: elems)
		es1.push_back(e->substitute(s));
	return std::make_shared<TupleExpr const>(es1);
}

Expr::Ref StringLit::substitute(Expr::Subst const& s) const
{
	(void)s;
	return std::make_shared<StringLit const>(val);
}

Expr::Ref DtSelExpr::substitute(Expr::Subst const& s) const
{
	Ref base1 = base->substitute(s);
	return std::make_shared<DtSelExpr const>(base1, member, constr, dt);
}

Expr::Ref DtUpdExpr::substitute(Expr::Subst const& s) const
{
	Ref base1 = base->substitute(s);
	Ref val1 = val->substitute(s);
	return std::make_shared<DtUpdExpr const>(base1, member, val1, constr, dt);
}

//
// Containment
//

bool ErrorExpr::contains(std::string) const
{
	return false;
}

bool BinExpr::contains(std::string id) const
{
	if (lhs->contains(id))
		return true;
	if (rhs->contains(id))
		return true;;
	return false;
}

bool CondExpr::contains(std::string id) const
{
	if (cond->contains(id))
		return true;
	if (then->contains(id))
		return true;
	if (else_->contains(id))
		return true;
	return false;
}

bool FunExpr::contains(std::string id) const
{
	for (Ref a: args)
		if (a->contains(id))
			return true;
	return false;
}

bool BoolLit::contains(std::string id) const
{
	(void) id;
	return false;
}

bool IntLit::contains(std::string id) const
{
	(void) id;
	return false;
}

bool BvLit::contains(std::string id) const
{
	(void) id;
	return false;
}

bool NegExpr::contains(std::string id) const
{
	return expr->contains(id);
}

bool NotExpr::contains(std::string id) const
{
	return expr->contains(id);
}

bool QuantExpr::contains(std::string id) const
{
	for (Binding b: vars)
	{
		auto varExpr = std::dynamic_pointer_cast<VarExpr const>(b.id);
		if (id == varExpr->getName())
			return false;
	}
	return expr->contains(id);
}

bool ArrConstExpr::contains(std::string id) const
{
	return val->contains(id);
}

bool ArrSelExpr::contains(std::string id) const
{
	if (base->contains(id))
		return true;
	if (idx->contains(id))
		return true;
	return false;
}

bool ArrUpdExpr::contains(std::string id) const
{
	if (base->contains(id))
		return true;
	if (idx->contains(id))
		return true;
	if (val->contains(id))
		return true;
	return false;
}

bool VarExpr::contains(std::string id) const
{
	return id == name;
}

bool OldExpr::contains(std::string id) const
{
	return expr->contains(id);
}

bool TupleExpr::contains(std::string id) const
{
	for (Ref e: elems)
		if (e->contains(id))
			return true;
	return false;
}

bool StringLit::contains(std::string id) const
{
	(void)id;
	return false;
}

bool DtSelExpr::contains(std::string id) const
{
	return base->contains(id);
}

bool DtUpdExpr::contains(std::string id) const
{
	if (base->contains(id))
		return true;
	if (val->contains(id))
		return true;
	return false;
}

//
// Comparison stuff
//

template<typename T>
struct CmpHelper {
	static int cmp(Expr::Ref e1, Expr::Ref e2)
	{
		auto ptr1 = dynamic_cast<T const*>(e1.get());
		solAssert(ptr1, "Wrong type");
		auto ptr2 = dynamic_cast<T const*>(e2.get());
		solAssert(ptr2, "Wrong type");
		return ptr1->cmp(*ptr2);
	}
};

int Expr::cmp(Expr::Ref e1, Expr::Ref e2)
{
	if (e1->kind() != e2->kind())
		return static_cast<int>(e1->kind()) - static_cast<int>(e2->kind());

	Kind kind = e1->kind();

	switch (kind)
	{
	case Kind::ERROR: return CmpHelper<ErrorExpr>::cmp(e1, e2);
	case Kind::EXISTS: return CmpHelper<QuantExpr>::cmp(e1, e2);
	case Kind::FORALL: return CmpHelper<QuantExpr>::cmp(e1, e2);
	case Kind::AND: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::OR: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::COND: return CmpHelper<CondExpr>::cmp(e1, e2);
	case Kind::EQ: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::LT: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::GT: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::LTE: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::GTE: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::PLUS: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::MINUS: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::SUB: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::DIV: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::INTDIV: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::TIMES: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::MOD: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::EXP: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::FN: return CmpHelper<FunExpr>::cmp(e1, e2);
	case Kind::VARIABLE: return CmpHelper<VarExpr>::cmp(e1, e2);
	case Kind::IMPL: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::IFF: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::LIT_BOOL: return CmpHelper<BoolLit>::cmp(e1, e2);
	case Kind::LIT_STRING: return CmpHelper<StringLit>::cmp(e1, e2);
	case Kind::LIT_INT: return CmpHelper<IntLit>::cmp(e1, e2);
	case Kind::LIT_BV: return CmpHelper<BvLit>::cmp(e1, e2);
	case Kind::NEQ: return CmpHelper<BinExpr>::cmp(e1, e2);
	case Kind::NOT: return CmpHelper<NotExpr>::cmp(e1, e2);
	case Kind::NEG: return CmpHelper<NegExpr>::cmp(e1, e2);
	case Kind::ARRAY_CONST: return CmpHelper<ArrConstExpr>::cmp(e1, e2);
	case Kind::ARRAY_SELECT: return CmpHelper<ArrSelExpr>::cmp(e1, e2);
	case Kind::ARRAY_UPDATE: return CmpHelper<ArrUpdExpr>::cmp(e1, e2);
	case Kind::DATATYPE_SELECT: return CmpHelper<DtSelExpr>::cmp(e1, e2);
	case Kind::DATATYPE_UPDATE: return CmpHelper<DtUpdExpr>::cmp(e1, e2);
	case Kind::OLD: return CmpHelper<OldExpr>::cmp(e1, e2);
	case Kind::TUPLE: return CmpHelper<TupleExpr>::cmp(e1, e2);
	case Kind::CONCAT: return CmpHelper<BinExpr>::cmp(e1, e2);
	}

	solAssert(false, "Unknown expression");
	return 0;
}

Expr::Kind BinExpr::kind() const {
	switch (op)
	{
	case Iff: return Kind::IFF;
	case Imp: return Kind::IMPL;
	case Or: return Kind::OR;
	case And: return Kind::AND;
	case Eq: return Kind::EQ;
	case Neq: return Kind::NEG;
	case Lt: return Kind::LT;
	case Gt: return Kind::GT;
	case Lte: return Kind::LTE;
	case Gte: return Kind::GTE;
	case Sub: return Kind::SUB;
	case Conc: return Kind::CONCAT;
	case Plus: return Kind::PLUS;
	case Minus: return Kind::MINUS;
	case Times: return Kind::TIMES;
	case Div: return Kind::DIV;
	case IntDiv: return Kind::INTDIV;
	case Mod: return Kind::MOD;
	case Exp: return Kind::EXP;
	}
	return Kind::ERROR;
}

Expr::Kind QuantExpr::kind() const
{
	switch (quant)
	{
	case Exists: return Kind::EXISTS;
	case Forall: return Kind::FORALL;
	}
	return Kind::ERROR;
}

int ErrorExpr::cmp(ErrorExpr const&) const
{
	return 0;
}

int BinExpr::cmp(BinExpr const& e) const
{
	solAssert(op == e.op, "Must be same binary expression");
	return Expr::cmp({lhs, rhs}, {e.lhs, e.rhs});
}

int CondExpr::cmp(CondExpr const& e) const
{
	return Expr::cmp({cond, then, else_}, {e.cond, e.then, e.else_});
}

int FunExpr::cmp(FunExpr const& e) const
{
	int cmp = std::strcmp(fun.c_str(), e.fun.c_str());
	if (cmp != 0) return cmp;
	if (args.size() != e.args.size())
		return static_cast<int>(args.size()) - static_cast<int>(e.args.size());
	return Expr::cmp(args, e.args);
}

int BoolLit::cmp(BoolLit const& e) const
{
	return static_cast<int>(val) - static_cast<int>(e.val);
}

int IntLit::cmp(IntLit const& e) const
{
	return val.compare(e.val);
}

int BvLit::cmp(BvLit const& e) const
{
	if (width != e.width)
		return static_cast<int>(width) - static_cast<int>(e.width);
	return std::strcmp(val.c_str(), e.val.c_str());
}

int NegExpr::cmp(NegExpr const& e) const
{
	return Expr::cmp(expr, e.expr);
}

int NotExpr::cmp(NotExpr const& e) const
{
	return Expr::cmp(expr, e.expr);
}

int QuantExpr::cmp(QuantExpr const& e) const
{
	solAssert(quant == e.quant, "Must be the same quantifier");

	if (vars.size() != e.vars.size())
		return static_cast<int>(vars.size()) - static_cast<int>(e.vars.size());

	for (unsigned i = 0; i < vars.size(); ++ i)
	{
		int cmp = Expr::cmp(vars[i].id, e.vars[i].id);
		if (cmp != 0)
			return cmp;
		cmp = vars[i].type->cmp(*e.vars[i].type.get());
		if (cmp != 0)
			return cmp;
	}

	return Expr::cmp(expr, e.expr);
}

int ArrConstExpr::cmp(ArrConstExpr const& e) const
{
	int cmp = arrType->cmp(*e.arrType.get());
	if (cmp != 0)
		return cmp;
	return Expr::cmp(val, e.val);
}

int ArrSelExpr::cmp(ArrSelExpr const& e) const
{
	return Expr::cmp({base, idx}, {e.base, e.idx});
}

int ArrUpdExpr::cmp(ArrUpdExpr const& e) const
{
	return Expr::cmp({base, idx, val}, {e.base, e.idx, e.val});
}

int VarExpr::cmp(VarExpr const& e) const
{
	return std::strcmp(name.c_str(), e.name.c_str());
}

int OldExpr::cmp(OldExpr const& e) const
{
	return Expr::cmp(expr, e.expr);
}

int TupleExpr::cmp(TupleExpr const& e) const
{
	if (elems.size() != e.elems.size())
		return static_cast<int>(elems.size()) - static_cast<int>(e.elems.size());
	return Expr::cmp(elems, e.elems);
}

int StringLit::cmp(StringLit const& e) const
{
	return std::strcmp(val.c_str(), e.val.c_str());
}

int DtSelExpr::cmp(DtSelExpr const& e) const
{
	int cmp = std::strcmp(member.c_str(), e.member.c_str());
	if (cmp != 0)
		return cmp;
	return Expr::cmp(base, e.base);
}

int DtUpdExpr::cmp(DtUpdExpr const& e) const
{
	int cmp = std::strcmp(member.c_str(), e.member.c_str());
	if (cmp != 0)
		return cmp;
	return Expr::cmp({base, val}, {e.base, e.val});
}

int Expr::cmp(std::vector<Ref> const& l1, std::vector<Ref> const& l2)
{
	solAssert(l1.size() == l2.size(), "Only vectors of same size");
	for (unsigned i = 0; i < l1.size(); ++ i)
	{
		int cmp = Expr::cmp(l1[i], l2[i]);
		if (cmp != 0)
			return cmp;
	}
	return 0;
}

}
