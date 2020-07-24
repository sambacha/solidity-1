//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/BoogieAstStmt.h>
#include <liblangutil/Exceptions.h>
#include <sstream>
#include <iostream>

namespace boogie {

Stmt::Ref Stmt::annot(std::vector<AttrConstRef> const& attrs)
{
	AssumeStmt* s = new AssumeStmt(Expr::true_());
	for (auto A: attrs)
		s->add(A);
	return std::shared_ptr<AssumeStmt const>(s);
}

Stmt::Ref Stmt::annot(AttrConstRef a)
{
	return Stmt::annot(std::vector<AttrConstRef>{a});
}

Stmt::Ref Stmt::assert_(Expr::Ref e, std::vector<AttrConstRef> const& attrs)
{
	return std::make_shared<AssertStmt const>(e, attrs);
}

Stmt::Ref Stmt::assign(Expr::Ref e, Expr::Ref f)
{
	if (auto cond = std::dynamic_pointer_cast<CondExpr const>(e))
		return Stmt::ifelse(cond->getCond(),
				Block::block("", {Stmt::assign(cond->getThen(), f)}),
				Block::block("", {Stmt::assign(cond->getElse(), f)}));
	if (auto sel = std::dynamic_pointer_cast<SelExpr const>(e))
	{
		auto upd = std::dynamic_pointer_cast<UpdExpr const>(Expr::selectToUpdate(sel, f));
		solAssert(upd, "Update expression expected");
		return Stmt::assign(upd->getBase(), upd);
	}
	return std::make_shared<AssignStmt const>(std::vector<Expr::Ref>{e}, std::vector<Expr::Ref>{f});
}

Stmt::Ref Stmt::assume(Expr::Ref e)
{
	return std::make_shared<AssumeStmt const>(e);
}

Stmt::Ref Stmt::assume(Expr::Ref e, AttrConstRef a)
{
	AssumeStmt* s = new AssumeStmt(e);
	s->add(a);
	return std::shared_ptr<AssumeStmt const>(s);
}

Stmt::Ref Stmt::call(std::string p, std::vector<Expr::Ref> const& args, std::vector<std::string> const& rets,
		std::vector<AttrConstRef> const& attrs)
{
	return std::make_shared<CallStmt const>(p, attrs, args, rets);
}

Stmt::Ref Stmt::comment(std::string s)
{
	return std::make_shared<CommentStmt const>(s);
}

Stmt::Ref Stmt::goto_(std::vector<std::string> const& ts)
{
	return std::make_shared<GotoStmt const>(ts);
}

Stmt::Ref Stmt::havoc(std::string x)
{
	return std::make_shared<HavocStmt const>(std::vector<std::string>{x});
}

Stmt::Ref Stmt::return_(Expr::Ref e)
{
	return std::make_shared<ReturnStmt const>(e);
}

Stmt::Ref Stmt::return_()
{
	return std::make_shared<ReturnStmt const>();
}

Stmt::Ref Stmt::skip()
{
	return std::make_shared<AssumeStmt const>(Expr::true_());
}

Stmt::Ref Stmt::ifelse(Expr::Ref cond, Block::ConstRef then, Block::ConstRef elze)
{
	return std::make_shared<IfElseStmt const>(cond, then, elze);
}

Stmt::Ref Stmt::while_(Expr::Ref cond, Block::ConstRef body, std::vector<Specification::Ref> const& invars)
{
	return std::make_shared<WhileStmt const>(cond, body, invars);
}

Stmt::Ref Stmt::break_()
{
	return std::make_shared<BreakStmt const>();
}

Stmt::Ref Stmt::label(std::string s)
{
	return std::make_shared<LabelStmt const>(s);
}

bool AssumeStmt::hasAttr(std::string name) const {
	for (auto a = attrs.begin(); a != attrs.end(); ++a) {
		if ((*a)->getName() == name)
			return true;
	}
	return false;
}

std::ostream& operator<<(std::ostream& os, Stmt::Ref s)
{
	s->print(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Block::ConstRef b)
{
	b->print(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Block::Ref b)
{
	b->print(os);
	return os;
}

void AssertStmt::print(std::ostream& os) const
{
	os << "assert ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << expr << ";";
}

void AssignStmt::print(std::ostream& os) const
{
	print_seq(os, lhs, ", ");
	os << " := ";
	print_seq(os, rhs, ", ");
	os << ";";
}

void AssumeStmt::print(std::ostream& os) const
{
	os << "assume ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << expr << ";";
}

void CallStmt::print(std::ostream& os) const
{
	os << "call ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	if (returns.size() > 0)
		print_seq(os, returns, "", ", ", " := ");
	os << proc;
	print_seq(os, params, "(", ", ", ")");
	os << ";";
}

void CommentStmt::print(std::ostream& os) const
{
	os << "// " << str;
}

void GotoStmt::print(std::ostream& os) const
{
	os << "goto ";
	print_seq(os, targets, ", ");
	os << ";";
}

void HavocStmt::print(std::ostream& os) const
{
	os << "havoc ";
	print_seq(os, vars, ", ");
	os << ";";
}

void ReturnStmt::print(std::ostream& os) const
{
	os << "return";
	if (expr)
		os << " " << expr;
	os << ";";
}

void IfElseStmt::print(std::ostream& os) const
{
	os << "if (";
	cond->printBg(os);
	os << ") {\n";
	then->print(os);
	os << "\n	}\n";

	if (elze)
	{
		os << "	else {\n";
		elze->print(os);
		os << "\n	}\n";
	}
}

void WhileStmt::print(std::ostream& os) const
{
	os << "while (";
	if (cond)
	{
		cond->printBg(os);
	}
	else
	{
		// Can be null in for loops when condition is omitted, e.g. for (;;) { break }
		os << "true";
	}
	os << ")";

	if (invars.empty())
	{
		os << " {\n";
	}
	else
	{
		os << "\n";
		for (auto inv: invars)
		{
			inv->print(os, "invariant");
			os << "\n";
		}
		os << "\n	{\n";
	}
	body->print(os);
	os << "\n	}\n";
}

void BreakStmt::print(std::ostream& os) const
{
	os << "break;";
}

void LabelStmt::print(std::ostream& os) const
{
	os << str << ":";
}

void Block::print(std::ostream& os) const
{
	if (name != "")
		os << name << ":" << "\n";
	print_seq(os, stmts, "	", "\n	", "");
}

}
