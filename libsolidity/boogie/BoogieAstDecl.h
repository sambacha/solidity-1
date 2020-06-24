//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <libsolidity/boogie/BoogieAstExpr.h>
#include <libsolidity/boogie/BoogieAstStmt.h>

namespace boogie
{
class Decl;
using DeclRef = std::shared_ptr<Decl>;
class TypeDecl;
using TypeDeclRef = std::shared_ptr<TypeDecl>;
class ProcDecl;
using ProcDeclRef = std::shared_ptr<ProcDecl>;
class FuncDecl;
using FuncDeclRef = std::shared_ptr<FuncDecl>;
class DataTypeDecl;
using DataTypeDeclRef = std::shared_ptr<DataTypeDecl>;
class VarDecl;
using VarDeclRef = std::shared_ptr<VarDecl>;

class Attr;
using AttrRef = std::shared_ptr<Attr const>;

class Specification;
using SpecificationRef = std::shared_ptr<Specification const>;

class CodeContainer {
protected:
	typedef std::vector<DeclRef> DeclarationList;
	typedef std::vector<Block::Ref> BlockList;
	typedef std::vector<std::string> ModifiesList;
	DeclarationList decls;
	BlockList blocks;
	ModifiesList mods;
	CodeContainer(DeclarationList ds, BlockList bs) : decls(ds), blocks(bs) {}
public:
	typedef DeclarationList::iterator decl_iterator;
	decl_iterator decl_begin() { return decls.begin(); }
	decl_iterator decl_end() { return decls.end(); }
	DeclarationList& getDeclarations() { return decls; }

	typedef BlockList::iterator iterator;
	iterator begin() { return blocks.begin(); }
	iterator end() { return blocks.end(); }
	BlockList& getBlocks() { return blocks; }

	typedef ModifiesList::iterator mod_iterator;
	mod_iterator mod_begin() { return mods.begin(); }
	mod_iterator mod_end() { return mods.end(); }
	ModifiesList& getModifies() { return mods; }
};

class Decl {
public:
	enum Kind {
		CONSTANT, VARIABLE, PROCEDURE, FUNCTION, TYPE, AXIOM, CODE, COMMENT
	};
private:
	Kind const kind;
public:
	Kind getKind() const { return kind; }
private:
	static unsigned uniqueId;
protected:
	unsigned id;
	std::string name;
	std::vector<AttrRef> attrs;
	Decl(Kind k, std::string n, std::vector<AttrRef> const& ax)
		: kind(k), id(uniqueId++), name(n), attrs(ax) { }
public:

	using Ref = std::shared_ptr<Decl>;
	using ConstRef = std::shared_ptr<Decl const>;

	virtual ~Decl() {}
	virtual void print(std::ostream& os) const = 0;
	unsigned getId() const { return id; }
	std::string getName() const { return name; }
	Expr::Ref getRefTo() const { return Expr::id(name); }
	void addAttr(AttrRef a) { attrs.push_back(a); }
	void addAttrs(std::vector<AttrRef> const& ax) { for (auto a: ax) addAttr(a); }

	static TypeDeclRef elementarytype(std::string name);
	static TypeDeclRef aliasedtype(std::string name, TypeDeclRef alias);
	static TypeDeclRef customtype(std::string name);
	static TypeDeclRef arraytype(TypeDeclRef keyType, TypeDeclRef valueType);
	static DataTypeDeclRef datatype(std::string name, std::vector<Binding> members);
	static Ref axiom(Expr::Ref e, std::string name = "");
	static FuncDeclRef function(
		std::string name,
		std::vector<Binding> const& args,
		TypeDeclRef type,
		Expr::Ref e = nullptr,
		std::vector<AttrRef> const& attrs = {});
	static Ref constant(std::string name, TypeDeclRef type);
	static Ref constant(std::string name, TypeDeclRef type, bool unique);
	static Ref constant(std::string name, TypeDeclRef type, std::vector<AttrRef> const& ax, bool unique);
	static VarDeclRef variable(std::string name, TypeDeclRef type);
	static ProcDeclRef procedure(std::string name,
		std::vector<Binding> const& params = {},
		std::vector<Binding> const& rets = {},
		std::vector<Ref> const& decls = {},
		std::vector<Block::Ref> const& blocks = {});
	static Ref code(std::string name, std::string s);
	static FuncDeclRef code(ProcDeclRef P);
	static Ref comment(std::string name, std::string str);
};

class TypeDecl : public Decl {
protected:
	std::string alias;
	std::string smttype;
public:
	TypeDecl(std::string n, std::string a, std::vector<AttrRef> const& ax, std::string smt)
		: Decl(TYPE, n, ax), alias(a), smttype(smt) {}
	std::string getAlias() const { return alias; }
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == TYPE; }
	std::string getSmtType() const { return smttype; }
	int cmp(TypeDecl const& td) const;
};

class DataTypeDecl : public TypeDecl {
	std::vector<Binding> members;
public:
	DataTypeDecl(std::string n, std::string t, std::vector<AttrRef> const& ax, std::vector<Binding> members);
	std::vector<Binding> getMembers() const { return members; }
	void print(std::ostream& os) const override;
};


class AxiomDecl : public Decl {
	Expr::Ref expr;
	static int uniqueId;
public:
	AxiomDecl(std::string n, Expr::Ref e) : Decl(AXIOM, n, {}), expr(e) {}
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == AXIOM; }
};

class ConstDecl : public Decl {
	TypeDeclRef type;
	bool unique;
public:
	ConstDecl(std::string n, TypeDeclRef t, std::vector<AttrRef> const& ax, bool u)
		: Decl(CONSTANT, n, ax), type(t), unique(u) {}
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == CONSTANT; }
};

class FuncDecl : public Decl {
	std::vector<Binding> params;
	TypeDeclRef type;
	Expr::Ref body;
public:
	FuncDecl(std::string n, std::vector<AttrRef> const& ax, std::vector<Binding> const& ps,
			TypeDeclRef t, Expr::Ref b)
		: Decl(FUNCTION, n, ax), params(ps), type(t), body(b) {}
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == FUNCTION; }
};

class VarDecl : public Decl {
	TypeDeclRef type;
public:
	VarDecl(std::string n, TypeDeclRef t) : Decl(VARIABLE, n, {}), type(t) {}
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == VARIABLE; }
	TypeDeclRef getType() const { return type; }
};

class ProcDecl : public Decl, public CodeContainer {
	typedef Binding Parameter;
	typedef std::vector<Parameter> ParameterList;
	typedef std::vector<SpecificationRef> SpecificationList;

	ParameterList params;
	ParameterList rets;
	SpecificationList requires;
	SpecificationList ensures;
public:
	ProcDecl(std::string n, ParameterList ps, ParameterList rs,
		DeclarationList ds, BlockList bs)
		: Decl(PROCEDURE, n, {}), CodeContainer(ds, bs), params(ps), rets(rs) {}
	typedef ParameterList::iterator param_iterator;
	param_iterator param_begin() { return params.begin(); }
	param_iterator param_end() { return params.end(); }
	ParameterList& getParameters() { return params; }

	param_iterator returns_begin() { return rets.begin(); }
	param_iterator returns_end() { return rets.end(); }
	ParameterList& getReturns() { return rets; }

	typedef SpecificationList::iterator spec_iterator;
	spec_iterator requires_begin() { return requires.begin(); }
	spec_iterator requires_end() { return requires.end(); }
	SpecificationList& getRequires() { return requires; }

	spec_iterator ensures_begin() { return ensures.begin(); }
	spec_iterator ensures_end() { return ensures.end(); }
	SpecificationList& getEnsures() { return ensures; }

	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == PROCEDURE; }
};

class CodeDecl : public Decl {
	std::string code;
public:
	CodeDecl(std::string name, std::string s) : Decl(CODE, name, {}), code(s) {}
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == CODE; }
};

class CommentDecl : public Decl {
	std::string str;
public:
	CommentDecl(std::string name, std::string str) : Decl(COMMENT, name, {}), str(str) {}
	void print(std::ostream& os) const override;
	static bool classof(Decl::ConstRef D) { return D->getKind() == COMMENT; }
};

std::ostream& operator<<(std::ostream& os, Decl& e);
std::ostream& operator<<(std::ostream& os, Decl::Ref e);
std::ostream& operator<<(std::ostream& os, Decl::ConstRef e);

}
