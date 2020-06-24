//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/BoogieAstDecl.h>
#include <libsolidity/boogie/BoogieAstExpr.h>
#include <liblangutil/Exceptions.h>
#include <iostream>

namespace boogie {

Attr::Ref Attr::attr(std::string s, std::vector<ExprRef> const& vs)
{
	return std::make_shared<Attr const>(s,vs);
}

Attr::Ref Attr::attr(std::string s)
{
	return attr(s, std::vector<ExprRef>());
}

Attr::Ref Attr::attr(std::string s, std::string v)
{
	return Attr::Ref(new Attr(s, { Expr::stringlit(v) }));
}

Attr::Ref Attr::attr(std::string s, int v)
{
	return attr(s, {Expr::intlit((long) v)});
}

Attr::Ref Attr::attr(std::string s, std::string v, int i)
{
	return attr(s, std::vector<ExprRef>{ Expr::stringlit(v), Expr::intlit((long) i) });
}

Attr::Ref Attr::attr(std::string s, std::string v, int i, int j)
{
	return attr(s, {Expr::stringlit(v), Expr::intlit((long) i), Expr::intlit((long) j)});
}

std::ostream& operator<<(std::ostream& os, Attr::Ref a)
{
	a->print(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Program const* p)
{
	if (p == 0)
		os << "<null> Program!\n";
	else
		p->print(os);
	return os;
}
std::ostream& operator<<(std::ostream& os, Program const& p)
{
	p.print(os);
	return os;
}

void Attr::print(std::ostream& os) const
{
	os << "{:" << name;
	if (vals.size() > 0)
		print_seq(os, vals, " ", ", ", "");
	os << "}";
}

Specification::Specification(ExprRef e, std::vector<Attr::Ref> const& ax)
: expr(e), attrs(ax)
{
	solAssert(e, "Specification cannot be null");
}

void Specification::print(std::ostream& os, std::string kind) const
{
	os << "	" << kind << " ";
	if (attrs.size() > 0)
			print_seq(os, attrs, "", " ", " ");
	os << expr << ";\n";
}

Specification::Ref Specification::spec(ExprRef e, std::vector<Attr::Ref> const& ax){
	return std::make_shared<Specification const>(e, ax);
}

Specification::Ref Specification::spec(ExprRef e){
	return Specification::spec(e, {});
}

void Program::print(std::ostream& os) const
{
	print_seq(os, decls, "\n");
	os << "\n";
}

}
