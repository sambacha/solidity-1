//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#include <libsolidity/boogie/BoogieAst.h>
#include <libsolidity/boogie/BoogieAstDecl.h>
#include <liblangutil/Exceptions.h>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>

namespace boogie {

unsigned Decl::uniqueId = 0;

TypeDeclRef Decl::elementarytype(std::string name)
{
	std::string smtname = name;
	if (name == "int" || name == "bool")
		smtname[0] = toupper(smtname[0]);
	if (boost::starts_with(name, "bv"))
		smtname = "(_ BitVec " + name.substr(2) + ")";
	return std::make_shared<TypeDecl>(name, "", std::vector<AttrRef>(), smtname);
}

TypeDeclRef Decl::aliasedtype(std::string name, TypeDeclRef alias)
{
	return std::make_shared<TypeDecl>(name, alias->getName(), std::vector<AttrRef>(), alias->getSmtType());
}

TypeDeclRef Decl::customtype(std::string name)
{
	return std::make_shared<TypeDecl>(name, "", std::vector<AttrRef>(), "T@" + name);
}

TypeDeclRef Decl::arraytype(TypeDeclRef keyType, TypeDeclRef valueType)
{
	return std::make_shared<TypeDecl>("[" + keyType->getName() + "]" + valueType->getName(),
			"", std::vector<AttrRef>(),
			"(Array " + keyType->getSmtType() + " " + valueType->getSmtType() + ")");
}

DataTypeDeclRef Decl::datatype(std::string name, std::vector<Binding> members)
{
	return std::make_shared<DataTypeDecl>(name, "", std::vector<AttrRef>(), members);
}

Decl::Ref Decl::axiom(Expr::Ref e, std::string name)
{
	return std::make_shared<AxiomDecl>(name, e);
}

FuncDeclRef Decl::function(std::string name, std::vector<Binding> const& args,
		TypeDeclRef type, Expr::Ref e, std::vector<AttrRef> const& attrs)
{
	return std::make_shared<FuncDecl>(name,attrs,args,type,e);
}

Decl::Ref Decl::constant(std::string name, TypeDeclRef type)
{
	return Decl::constant(name, type, {}, false);
}

Decl::Ref Decl::constant(std::string name, TypeDeclRef type, bool unique)
{
	return Decl::constant(name, type, {}, unique);
}

Decl::Ref Decl::constant(std::string name, TypeDeclRef type, std::vector<AttrRef> const& ax, bool unique)
{
	return std::make_shared<ConstDecl>(name, type, ax, unique);
}

VarDeclRef Decl::variable(std::string name, TypeDeclRef type)
{
	return std::make_shared<VarDecl>(name, type);
}

ProcDeclRef Decl::procedure(std::string name,
		std::vector<Binding> const& args,
		std::vector<Binding> const& rets,
		std::vector<Decl::Ref> const& decls,
		std::vector<Block::Ref> const& blocks)
{
	return std::make_shared<ProcDecl>(name, args, rets, decls, blocks);
}
Decl::Ref Decl::code(std::string name, std::string s)
{
	return std::make_shared<CodeDecl>(name, s);
}

Decl::Ref Decl::comment(std::string name, std::string str)
{
	return std::make_shared<CommentDecl>(name, str);
}

DataTypeDecl::DataTypeDecl(std::string n, std::string t, std::vector<AttrRef> const& ax, std::vector<Binding> members)
		: TypeDecl(n, t, ax, "|T@" + n + "|"), members(members) { attrs.push_back(Attr::attr("datatype")); }

std::ostream& operator<<(std::ostream& os, Decl& d)
{
	d.print(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Decl::Ref d)
{
	d->print(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, Decl::ConstRef d)
{
	d->print(os);
	return os;
}

void TypeDecl::print(std::ostream& os) const
{
	os << "type ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << name;
	if (alias != "")
		os << " = " << alias;
	os << ";";
}

void DataTypeDecl::print(std::ostream& os) const
{
	os << "type ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << name;
	if (alias != "")
		os << " = " << alias;
	os << ";";
}

void AxiomDecl::print(std::ostream& os) const
{
	os << "axiom ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << expr << ";";
}

void ConstDecl::print(std::ostream& os) const
{
	os << "const ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << (unique ? "unique " : "") << name << ": " << type->getName() << ";";
}

void FuncDecl::print(std::ostream& os) const
{
	os << "function ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << name << "(";
	for (auto P = params.begin(), E = params.end(); P != E; ++P)
	{
		std::stringstream pname;
		pname << *(P->id);
		os << (P == params.begin() ? "" : ", ") << (pname.str() != "" ? pname.str() + ": " : "") << P->type->getName();
	}
	os << ") returns (" << type->getName() << ")";
	if (body)
		os << " { " << body << " }";
	else
		os << ";";
}

void VarDecl::print(std::ostream& os) const
{
	os << "var ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << name << ": " << type->getName() << ";";
}

void ProcDecl::print(std::ostream& os) const
{
	os << "procedure ";
	if (attrs.size() > 0)
		print_seq(os, attrs, "", " ", " ");
	os << name << "(";
	for (auto P = params.begin(), E = params.end(); P != E; ++P)
		os << (P == params.begin() ? "" : ", ") << P->id << ": " << P->type->getName();
	os << ")";
	if (rets.size() > 0)
	{
		os << "\n";
		os << "	returns (";
		for (auto R = rets.begin(), E = rets.end(); R != E; ++R)
			os << (R == rets.begin() ? "" : ", ") << R->id << ": " << R->type->getName();
		os << ")";
	}
	if (blocks.size() == 0)
		os << ";";

	if (mods.size() > 0)
	{
		os << "\n";
		print_seq(os, mods, "	modifies ", ", ", ";");
	}
	if (requires.size() > 0)
	{
		os << "\n";
		for (auto req: requires) req->print(os, "requires");
	}
	if (ensures.size() > 0)
	{
		os << "\n";
		for (auto ens: ensures) ens->print(os, "ensures");
	}
	if (blocks.size() > 0)
	{
		os << "\n";
		os << "{" << "\n";
		if (decls.size() > 0)
			print_seq(os, decls, "	", "\n	", "\n");
		print_seq(os, blocks, "\n");
		os << "\n" << "}";
	}
	os << "\n";
}

void CodeDecl::print(std::ostream& os) const
{
	os << code;
}

void CommentDecl::print(std::ostream& os) const
{
	os << "// ";
	for (char c: str)
	{
		os << c;
		if (c == '\n')
			os << "// ";
	}
}

int TypeDecl::cmp(TypeDecl const& td) const
{
	return std::strcmp(smttype.c_str(), td.smttype.c_str());
}

}
