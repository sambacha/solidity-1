#pragma once
#include <libsolidity/ast/Types.h>
#include <libsolidity/boogie/BoogieAst.h>

namespace dev
{
namespace solidity
{
class BoogieContext;

class StoragePtrHelper {

public:
	/**
	 * Packs an expression into a local storage pointer.
	 * @param expr Expression to be packed
	 * @param bgExpr Corresponding Boogie expression
	 * @param context Context
	 * @returns Local pointer
	 */
	static
	boogie::Expr::Ref packToLocalPtr(Expression const* expr, boogie::Expr::Ref bgExpr, BoogieContext& context);

	/**
	 * Unpacks a local storage pointer into an access to storage.
	 * @param ptrExpr Expression to be unpacked
	 * @param ptrBgExpr Corresponding Boogie expression
	 * @param context Context
	 */
	static
	boogie::Expr::Ref unpackLocalPtr(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, BoogieContext& context);

private:

	/** Helper struct for internal operations during packing. */
	struct PackResult {
		std::vector<boogie::Expr::Ref> conds; // Conditions
		std::vector<std::vector<boogie::Expr::Ref>> exprs; // Elements in the packed array
	};

	/**
	 * Transform a list of integer expressions into a series of array writes.
	 * For example, [3, i, 4] becomes constarray()[0 <- 3][1 <- i][2 <- 4].
	 */
	static
	boogie::Expr::Ref toWriteExpr(std::vector<boogie::Expr::Ref> exprs, BoogieContext& context);

	/**
	 * Packs a path to a local storage into an array. E.g., 'ss[i].t' becomes [2, i, 3] if
	 *  'ss' is the 2nd state var and 't' is the 3rd member.
	 */
	static
	void packInternal(Expression const* expr, boogie::Expr::Ref bgExpr, BoogieContext& context, PackResult& result);

	/**
	 * Unpacks an array into a tree of paths (conditional) to local storage (opposite of packing).
	 * E.g., ite(arr[0] == 0, statevar1, ite(arr[0] == 1, statevar2, ...
	 */
	static
	boogie::Expr::Ref unpackInternal(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, Declaration const* decl, int depth, boogie::Expr::Ref base, BoogieContext& context);

	static
	PackResult repack(Expression const* ptrExpr, boogie::Expr::Ref ptrBgExpr, Declaration const* decl, int depth, BoogieContext& context);
};

}

}
