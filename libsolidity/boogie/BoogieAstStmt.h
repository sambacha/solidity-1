//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <libsolidity/boogie/BoogieAstExpr.h>

namespace boogie
{
class Block;
using BlockConstRef = std::shared_ptr<Block const>;

class Attr;
using AttrConstRef = std::shared_ptr<Attr const>;

class Specification;
using SpecificationRef = std::shared_ptr<Specification const>;

class Stmt {
public:
	enum Kind {
		ASSERT, ASSUME, ASSIGN, HAVOC, GOTO, CALL, RETURN, COMMENT, IFELSE, WHILE, BREAK, LABEL
	};

	using Ref = std::shared_ptr<Stmt const>;

private:
	Kind const kind;
protected:
	Stmt(Kind k) : kind(k) {}
public:
	Kind getKind() const { return kind; }

public:
	virtual ~Stmt() {}
	static Ref annot(std::vector<AttrConstRef> const& attrs);
	static Ref annot(AttrConstRef a);
	static Ref assert_(Expr::Ref e,
		std::vector<AttrConstRef> const& attrs = {});
	static Ref assign(Expr::Ref e, Expr::Ref f);
	static Ref assume(Expr::Ref e);
	static Ref assume(Expr::Ref e, AttrConstRef attr);
	static Ref call(
		std::string p,
		std::vector<Expr::Ref> const& args = {},
		std::vector<std::string> const& rets = {},
		std::vector<AttrConstRef> const& attrs = {});
	static Ref comment(std::string c);
	static Ref goto_(std::vector<std::string> const& ts);
	static Ref havoc(std::string x);
	static Ref return_();
	static Ref return_(Expr::Ref e);
	static Ref skip();
	static Ref ifelse(Expr::Ref cond, BlockConstRef then, BlockConstRef elze = nullptr);
	static Ref while_(
			Expr::Ref cond,
			BlockConstRef body,
			std::vector<SpecificationRef> const& invars = {});
	static Ref break_();
	static Ref label(std::string name);
	virtual void print(std::ostream& os) const = 0;
};

class AssertStmt : public Stmt {
	Expr::Ref expr;
	std::vector<AttrConstRef> attrs;
public:
	AssertStmt(Expr::Ref e, std::vector<AttrConstRef> const& ax)
		: Stmt(ASSERT), expr(e), attrs(ax) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == ASSERT; }
};

class AssignStmt : public Stmt {
	std::vector<Expr::Ref> lhs;
	std::vector<Expr::Ref> rhs;
public:
	AssignStmt(std::vector<Expr::Ref> const& lhs, std::vector<Expr::Ref> const& rhs)
		: Stmt(ASSIGN), lhs(lhs), rhs(rhs) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == ASSIGN; }
};

class AssumeStmt : public Stmt {
	Expr::Ref expr;
	std::vector<AttrConstRef> attrs;
public:
	AssumeStmt(Expr::Ref e) : Stmt(ASSUME), expr(e) {}
	void add(AttrConstRef a) {
		attrs.push_back(a);
	}
	bool hasAttr(std::string name) const;
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == ASSUME; }
};

class CallStmt : public Stmt {
	std::string proc;
	std::vector<AttrConstRef> attrs;
	std::vector<Expr::Ref> params;
	std::vector<std::string> returns;
public:
	CallStmt(std::string p,
		std::vector<AttrConstRef> const& attrs,
		std::vector<Expr::Ref> const& args,
		std::vector<std::string> const& rets)
		: Stmt(CALL), proc(p), attrs(attrs), params(args), returns(rets) {}

	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == CALL; }
};

class CommentStmt : public Stmt {
	std::string str;
public:
	CommentStmt(std::string s) : Stmt(COMMENT), str(s) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == COMMENT; }
};

class GotoStmt : public Stmt {
	std::vector<std::string> targets;
public:
	GotoStmt(std::vector<std::string> const& ts) : Stmt(GOTO), targets(ts) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == GOTO; }
};

class HavocStmt : public Stmt {
	std::vector<std::string> vars;
public:
	HavocStmt(std::vector<std::string> const& vs) : Stmt(HAVOC), vars(vs) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == HAVOC; }
};

class ReturnStmt : public Stmt {
	Expr::Ref expr;
public:
	ReturnStmt(Expr::Ref e = nullptr) : Stmt(RETURN), expr(e) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == RETURN; }
};

class IfElseStmt : public Stmt {
	Expr::Ref cond;
	BlockConstRef then;
	BlockConstRef elze;
public:
	IfElseStmt(Expr::Ref cond, BlockConstRef then, BlockConstRef elze)
		: Stmt(IFELSE), cond(cond), then(then), elze(elze) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == IFELSE; }
};

class WhileStmt : public Stmt {
	Expr::Ref cond;
	BlockConstRef body;
	std::vector<SpecificationRef> invars;
public:
	WhileStmt(Expr::Ref cond, BlockConstRef body, std::vector<SpecificationRef> const& invars)
		: Stmt(WHILE), cond(cond), body(body), invars(invars) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == WHILE; }
};

class BreakStmt : public Stmt {
public:
	BreakStmt() : Stmt(BREAK) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == BREAK; }
};

class LabelStmt : public Stmt {
	std::string str;
public:
	LabelStmt(std::string s) : Stmt(LABEL), str(s) {}
	void print(std::ostream& os) const override;
	static bool classof(Ref S) { return S->getKind() == LABEL; }
};

class Block {
	std::string name;
	typedef std::vector<Stmt::Ref> StatementList;
	StatementList stmts;
public:

	using Ref = std::shared_ptr<Block>;
	using ConstRef = std::shared_ptr<Block const>;

	static
	Ref block(std::string n = "", std::vector<Stmt::Ref> const& stmts = {}) {
		return std::make_shared<Block>(n,stmts);
	}

	Block(std::string n, std::vector<Stmt::Ref> const& stmts) : name(n), stmts(stmts) {}
	void print(std::ostream& os) const;
	typedef StatementList::iterator iterator;
	iterator begin() { return stmts.begin(); }
	iterator end() { return stmts.end(); }
	StatementList& getStatements() { return stmts; }
	void addStmt(Stmt::Ref s) { stmts.push_back(s); }

	void addStmts(std::vector<Stmt::Ref> const& stmts) {
		for (auto s: stmts)
			addStmt(s);
	}

	std::string getName() { return name; }
};

std::ostream& operator<<(std::ostream& os, Stmt::Ref s);
std::ostream& operator<<(std::ostream& os, Block::ConstRef b);
std::ostream& operator<<(std::ostream& os, Block::Ref b);

}
