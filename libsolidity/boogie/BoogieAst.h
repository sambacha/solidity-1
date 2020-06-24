//
// This file is distributed under the MIT License. See SMACK-LICENSE for details.
//
#pragma once

#include <string>
#include <vector>
#include <set>
#include <memory>

namespace boogie
{

class Expr;
using ExprRef = std::shared_ptr<Expr const>;
class Decl;
using DeclRef = std::shared_ptr<Decl>;

class Attr {
protected:
	std::string name;
	std::vector<ExprRef> vals;
public:

	using Ref = std::shared_ptr<Attr const>;

	Attr(std::string n, std::vector<ExprRef> const& vs) : name(n), vals(vs) {}
	void print(std::ostream& os) const;
	std::string getName() const { return name; }

	static Ref attr(std::string s);
	static Ref attr(std::string s, std::string v);
	static Ref attr(std::string s, int v);
	static Ref attr(std::string s, std::string v, int i);
	static Ref attr(std::string s, std::string v, int i, int j);
	static Ref attr(std::string s, std::vector<ExprRef> const& vs);
};

class Specification {
	ExprRef expr;
	std::vector<Attr::Ref> attrs;
public:

	using Ref = std::shared_ptr<Specification const>;

	Specification(ExprRef e, std::vector<Attr::Ref> const& ax);

	void print(std::ostream& os, std::string kind) const;
	static Ref spec(ExprRef e, std::vector<Attr::Ref> const& ax);
	static Ref spec(ExprRef e);
};

class Program {
	typedef std::vector<DeclRef> DeclarationList;
	DeclarationList decls;
public:
	Program() {}
	void print(std::ostream& os) const;
	typedef DeclarationList::iterator iterator;
	iterator begin() { return decls.begin(); }
	iterator end() { return decls.end(); }
	unsigned size() { return decls.size(); }
	bool empty() { return decls.empty(); }
	DeclarationList& getDeclarations() { return decls; }
};

template<class T>
void print_seq(std::ostream& os, std::vector<T> const& ts, std::string init, std::string sep, std::string term)
{
	os << init;
	for (auto i = ts.begin(); i != ts.end(); ++i)
		os << (i == ts.begin() ? "" : sep) << *i;
	os << term;
}

template<class T>
void print_seq(std::ostream& os, std::vector<T> const& ts, std::string sep)
{
	print_seq<T>(os, ts, "", sep, "");
}

template<class T>
void print_seq(std::ostream& os, std::vector<T> const& ts)
{
	print_seq<T>(os, ts, "", "", "");
}

template<class T, class C>
void print_set(std::ostream& os, std::set<T,C> const& ts, std::string init, std::string sep, std::string term)
{
	os << init;
	for (typename std::set<T,C>::const_iterator i = ts.begin(); i != ts.end(); ++i)
		os << (i == ts.begin() ? "" : sep) << *i;
	os << term;
}

template<class T, class C>
void print_set(std::ostream& os, std::set<T,C> const& ts, std::string sep)
{
	print_set<T,C>(os, ts, "", sep, "");
}

template<class T, class C>
void print_set(std::ostream& os, std::set<T,C> const& ts)
{
	print_set<T,C>(os, ts, "", "", "");
}

}
