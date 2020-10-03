#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <libsolidity/ast/AST.h>
#include <libsolidity/boogie/ASTBoogieUtils.h>
#include <libsolidity/boogie/BoogieContext.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/TypeProvider.h>
#include <liblangutil/SourceLocation.h>
#include <liblangutil/Exceptions.h>
#include <liblangutil/Scanner.h>

using namespace std;
using namespace solidity;
using namespace solidity::frontend;
using namespace langutil;

namespace bg = boogie;

namespace solidity
{
namespace frontend
{

ASTBoogieUtils::SolidityID const ASTBoogieUtils::BALANCE = { "balance", "__balance" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::TRANSFER = { "transfer", "__transfer" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::SEND = { "send", "__send" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::CALL = { "call", "__call" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::SUPER = { "super", "__super" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::THIS = { "this", "__this" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::SENDER = { "sender", "__msg_sender" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::VALUE = { "value", "__msg_value" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::NOW = { "now", "__now" };
ASTBoogieUtils::SolidityID const ASTBoogieUtils::BLOCKNO = { "number", "__block_number" };

string const ASTBoogieUtils::SOLIDITY_ASSERT = "assert";
string const ASTBoogieUtils::SOLIDITY_REQUIRE = "require";
string const ASTBoogieUtils::SOLIDITY_REVERT = "revert";

string const ASTBoogieUtils::SOLIDITY_KECCAK256 = "keccak256";
string const ASTBoogieUtils::SOLIDITY_SHA256 = "sha256";
string const ASTBoogieUtils::SOLIDITY_RIPEMD160 = "ripemd160";
string const ASTBoogieUtils::SOLIDITY_ECRECOVER = "ecrecover";

string const ASTBoogieUtils::VERIFIER_SUM = "__verifier_sum";
string const ASTBoogieUtils::VERIFIER_IDX = "__verifier_idx";
string const ASTBoogieUtils::VERIFIER_OLD = "__verifier_old";
string const ASTBoogieUtils::VERIFIER_BEFORE = "__verifier_before";
string const ASTBoogieUtils::VERIFIER_EQ = "__verifier_eq";
string const ASTBoogieUtils::BOOGIE_CONSTRUCTOR = "__constructor";
string const ASTBoogieUtils::BOOGIE_ALLOC_COUNTER = "__alloc_counter";
string const ASTBoogieUtils::VERIFIER_OVERFLOW = "__verifier_overflow";

string const ASTBoogieUtils::DOCTAG_CONTRACT_INVAR = "invariant";
string const ASTBoogieUtils::DOCTAG_CONTRACT_INVARS_INCLUDE = "{contractInvariants}";
string const ASTBoogieUtils::DOCTAG_LOOP_INVAR = "invariant";
string const ASTBoogieUtils::DOCTAG_PRECOND = "precondition";
string const ASTBoogieUtils::DOCTAG_POSTCOND = "postcondition";
string const ASTBoogieUtils::DOCTAG_SPECIFICATION_CASES = "specification";
string const ASTBoogieUtils::DOCTAG_MODIFIES = "modifies";
string const ASTBoogieUtils::DOCTAG_MODIFIES_ALL = DOCTAG_MODIFIES + " *";
string const ASTBoogieUtils::DOCTAG_MODIFIES_COND = " if ";
string const ASTBoogieUtils::DOCTAG_EMITS = "emits";
string const ASTBoogieUtils::DOCTAG_EVENT_TRACKS_CHANGES = "tracks-changes-in";
string const ASTBoogieUtils::DOCTAG_EVENT_ALLOW_NO_CHANGE_EMIT = "allow-no-change-emit";

bg::ProcDeclRef ASTBoogieUtils::createTransferProc(BoogieContext& context)
{
	// Parameters: this, msg.sender, msg.value, amount
	vector<bg::Binding> transferParams{
		{context.boogieThis()->getRefTo(), context.boogieThis()->getType() },
		{context.boogieMsgSender()->getRefTo(), context.boogieMsgSender()->getType() },
		{context.boogieMsgValue()->getRefTo(), context.boogieMsgValue()->getType() },
		{bg::Expr::id("amount"), context.intType(256) }
	};

	// Type to pass around
	TypePointer tp_uint256 = TypeProvider::integer(256, IntegerType::Modifier::Unsigned);

	// Body
	bg::Block::Ref transferImpl = bg::Block::block();
	bg::Expr::Ref this_bal = bg::Expr::arrsel(context.boogieBalance()->getRefTo(), context.boogieThis()->getRefTo());
	bg::Expr::Ref sender_bal = bg::Expr::arrsel(context.boogieBalance()->getRefTo(), context.boogieMsgSender()->getRefTo());
	bg::Expr::Ref amount = bg::Expr::id("amount");

	// Precondition: there is enough ether to transfer
	auto geqResult = encodeArithBinaryOp(context, nullptr, langutil::Token::GreaterThanOrEqual, sender_bal, amount, 256, false);
	transferImpl->addStmt(bg::Stmt::assume(geqResult.expr));
	// balance[this] += amount
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		transferImpl->addStmts({
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(this_bal, tp_uint256)),
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(amount, tp_uint256))
		});
	}
	auto addBalance = encodeArithBinaryOp(context, nullptr, Token::Add, this_bal, amount, 256, false);
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		transferImpl->addStmts({
			bg::Stmt::comment("Implicit assumption that balances cannot overflow"),
			bg::Stmt::assume(addBalance.cc)
		});
	}
	transferImpl->addStmt(bg::Stmt::assign(
			context.boogieBalance()->getRefTo(),
			bg::Expr::arrupd(context.boogieBalance()->getRefTo(), context.boogieThis()->getRefTo(), addBalance.expr)));
	// balance[msg.sender] -= amount
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		transferImpl->addStmt(bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(sender_bal, tp_uint256)));
		transferImpl->addStmt(bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(amount, tp_uint256)));
	}
	auto subSenderBalance = encodeArithBinaryOp(context, nullptr, Token::Sub, sender_bal, amount, 256, false);
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		transferImpl->addStmt(bg::Stmt::comment("Implicit assumption that balances cannot overflow"));
		transferImpl->addStmt(bg::Stmt::assume(subSenderBalance.cc));
	}
	transferImpl->addStmt(bg::Stmt::assign(
			context.boogieBalance()->getRefTo(),
			bg::Expr::arrupd(context.boogieBalance()->getRefTo(), context.boogieMsgSender()->getRefTo(), subSenderBalance.expr)));
	transferImpl->addStmt(bg::Stmt::comment("TODO: call fallback, exception handling"));

	bg::ProcDeclRef transfer = bg::Decl::procedure(TRANSFER.boogie, transferParams, {}, {}, {transferImpl});

	transfer->addAttrs({
		bg::Attr::attr("inline", 1),
		bg::Attr::attr("message", "transfer")
	});
	return transfer;
}

bg::ProcDeclRef ASTBoogieUtils::createCallProc(BoogieContext& context)
{

	// Parameters: this, msg.sender, msg.value
	vector<bg::Binding> callParams {
		{context.boogieThis()->getRefTo(), context.boogieThis()->getType() },
		{context.boogieMsgSender()->getRefTo(), context.boogieMsgSender()->getType() },
		{context.boogieMsgValue()->getRefTo(), context.boogieMsgValue()->getType() }
	};

	// Type to pass around
	TypePointer tp_uint256 = TypeProvider::integer(256, IntegerType::Modifier::Unsigned);

	// Get the type of the call function
	auto callType = TypeProvider::address()->memberType("call");
	auto callFunctionType = dynamic_cast<FunctionType const*>(callType);

	solAssert(callFunctionType, "");
	solAssert(callFunctionType->returnParameterTypes().size() == 2, "");

	// Return value
	vector<bg::Binding> callReturns{
		{bg::Expr::id("__result"), context.toBoogieType(callFunctionType->returnParameterTypes()[0], nullptr)},
		{bg::Expr::id("__calldata"), context.toBoogieType(callFunctionType->returnParameterTypes()[1], nullptr)}
	};

	// Body
	// Successful transfer
	bg::Block::Ref thenBlock = bg::Block::block();
	bg::Expr::Ref this_bal = bg::Expr::arrsel(context.boogieBalance()->getRefTo(), context.boogieThis()->getRefTo());
	bg::Expr::Ref msg_val = context.boogieMsgValue()->getRefTo();
	bg::Expr::Ref result = bg::Expr::id("__result");

	// balance[this] += msg.value
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		thenBlock->addStmts({
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(this_bal, tp_uint256)),
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(msg_val, tp_uint256))
		});
	}
	auto addBalance = encodeArithBinaryOp(context, nullptr, Token::Add, this_bal, msg_val, 256, false);
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		thenBlock->addStmts({
			bg::Stmt::comment("Implicit assumption that balances cannot overflow"),
			bg::Stmt::assume(addBalance.cc)
		});
	}
	thenBlock->addStmt(bg::Stmt::assign(
			context.boogieBalance()->getRefTo(),
			bg::Expr::arrupd(context.boogieBalance()->getRefTo(), context.boogieThis()->getRefTo(), addBalance.expr)));
	thenBlock->addStmt(bg::Stmt::assign(result, bg::Expr::true_()));
	// Unsuccessful transfer
	bg::Block::Ref elseBlock = bg::Block::block();
	elseBlock->addStmt(bg::Stmt::assign(result, bg::Expr::false_()));
	// Nondeterministic choice between success and failure
	bg::Block::Ref callBlock = bg::Block::block();
	callBlock->addStmt(bg::Stmt::comment("TODO: call fallback"));
	callBlock->addStmt(bg::Stmt::ifelse(bg::Expr::id("*"), thenBlock, elseBlock));

	bg::ProcDeclRef callProc = bg::Decl::procedure(CALL.boogie, callParams, callReturns, {}, {callBlock});
	callProc->addAttr(bg::Attr::attr("inline", 1));
	callProc->addAttr(bg::Attr::attr("message", "call"));
	return callProc;
}

bg::ProcDeclRef ASTBoogieUtils::createSendProc(BoogieContext& context)
{
	bg::Expr::Ref amount = bg::Expr::id("amount");
	bg::Expr::Ref result = bg::Expr::id("__result");
	// Parameters: this, msg.sender, msg.value, amount
	vector<bg::Binding> sendParams {
		{context.boogieThis()->getRefTo(), context.boogieThis()->getType()},
		{context.boogieMsgSender()->getRefTo(), context.boogieMsgSender()->getType()},
		{context.boogieMsgValue()->getRefTo(), context.boogieMsgValue()->getType()},
		{amount, context.intType(256)}
	};

	// Type to pass around
	TypePointer tp_uint256 = TypeProvider::integer(256, IntegerType::Modifier::Unsigned);

	// Return value
	vector<bg::Binding> sendReturns{ {result, context.boolType()} };

	// Body
	// Successful transfer
	bg::Block::Ref thenBlock = bg::Block::block();
	bg::Expr::Ref this_bal = bg::Expr::arrsel(context.boogieBalance()->getRefTo(), context.boogieThis()->getRefTo());
	bg::Expr::Ref sender_bal = bg::Expr::arrsel(context.boogieBalance()->getRefTo(), context.boogieMsgSender()->getRefTo());

	// balance[this] += amount
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		thenBlock->addStmts({
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(this_bal, tp_uint256)),
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(amount, tp_uint256))
		});
	}
	auto addBalance = encodeArithBinaryOp(context, nullptr, Token::Add, this_bal, amount, 256, false);
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		thenBlock->addStmts({
			bg::Stmt::comment("Implicit assumption that balances cannot overflow"),
			bg::Stmt::assume(addBalance.cc)
		});
	}
	thenBlock->addStmt(bg::Stmt::assign(
			context.boogieBalance()->getRefTo(),
			bg::Expr::arrupd(context.boogieBalance()->getRefTo(), context.boogieThis()->getRefTo(), addBalance.expr)));
	// balance[msg.sender] -= amount
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		thenBlock->addStmts({
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(sender_bal, tp_uint256)),
			bg::Stmt::assume(ASTBoogieUtils::getTCCforExpr(amount, tp_uint256))
		});
	}
	auto subSenderBalance = encodeArithBinaryOp(context, nullptr, Token::Sub, sender_bal, amount, 256, false);
	if (context.encoding() == BoogieContext::Encoding::MOD)
	{
		thenBlock->addStmts({
			bg::Stmt::comment("Implicit assumption that balances cannot overflow"),
			bg::Stmt::assume(subSenderBalance.cc)
		});
	}
	thenBlock->addStmt(bg::Stmt::assign(
			context.boogieBalance()->getRefTo(),
			bg::Expr::arrupd(context.boogieBalance()->getRefTo(), context.boogieMsgSender()->getRefTo(), subSenderBalance.expr)));
	thenBlock->addStmt(bg::Stmt::assign(result, bg::Expr::true_()));

	// Unsuccessful transfer
	bg::Block::Ref elseBlock = bg::Block::block();
	elseBlock->addStmt(bg::Stmt::assign(result, bg::Expr::false_()));

	bg::Block::Ref transferBlock = bg::Block::block();
	// Precondition: there is enough ether to transfer
	auto senderBalanceGEQ = encodeArithBinaryOp(context, nullptr, langutil::Token::GreaterThanOrEqual, sender_bal, amount, 256, false);
	transferBlock->addStmt(bg::Stmt::assume(senderBalanceGEQ.expr));

	// Nondeterministic choice between success and failure
	transferBlock->addStmts({
		bg::Stmt::comment("TODO: call fallback"),
		bg::Stmt::ifelse(bg::Expr::id("*"), thenBlock, elseBlock)
	});

	bg::ProcDeclRef sendProc = bg::Decl::procedure(SEND.boogie, sendParams, sendReturns, {}, {transferBlock});

	sendProc->addAttrs({
		bg::Attr::attr("inline", 1),
		bg::Attr::attr("message", "send")
	});

	return sendProc;
}

string ASTBoogieUtils::dataLocToStr(DataLocation loc)
{
	switch (loc)
	{
	case DataLocation::Storage: return "storage";
	case DataLocation::Memory: return "memory";
	case DataLocation::CallData: return "calldata";
	default:
		solAssert(false, "Unknown storage location.");
	}
	return "";
}

string ASTBoogieUtils::getConstructorName(ContractDefinition const* contract)
{
	return BOOGIE_CONSTRUCTOR + "#" + util::toString(contract->id());
}

std::vector<bg::Attr::Ref> ASTBoogieUtils::createAttrs(SourceLocation const& loc, std::string const& message, Scanner const& scanner)
{
	int srcLine, srcCol;
	tie(srcLine, srcCol) = scanner.translatePositionToLineColumn(loc.start);
	return {
		bg::Attr::attr("sourceloc", loc.source->name(), srcLine + 1, srcCol + 1),
		bg::Attr::attr("message", message)
	};
}

ASTBoogieUtils::ExprWithCC ASTBoogieUtils::encodeArithBinaryOp(BoogieContext& context, ASTNode const* associatedNode, langutil::Token op,
		bg::Expr::Ref lhs, bg::Expr::Ref rhs, unsigned bits, bool isSigned)
{
	bg::Expr::Ref result = nullptr;
	bg::Expr::Ref ecc = nullptr;

	switch (context.encoding())
	{
	case BoogieContext::Encoding::INT:
		switch (op)
		{
		case Token::Add:
		case Token::AssignAdd:
			result = bg::Expr::plus(lhs, rhs); break;
		case Token::Sub:
		case Token::AssignSub:
			result = bg::Expr::minus(lhs, rhs); break;
		case Token::Mul:
		case Token::AssignMul:
			result = bg::Expr::times(lhs, rhs); break;
		// TODO: returning integer division is fine, because Solidity does not support floats
		case Token::Div:
		case Token::AssignDiv:
			result = bg::Expr::intdiv(lhs, rhs); break;
		case Token::Mod:
		case Token::AssignMod:
			result = bg::Expr::mod(lhs, rhs); break;

		case Token::LessThan: result = bg::Expr::lt(lhs, rhs); break;
		case Token::GreaterThan: result = bg::Expr::gt(lhs, rhs); break;
		case Token::LessThanOrEqual: result = bg::Expr::lte(lhs, rhs); break;
		case Token::GreaterThanOrEqual: result = bg::Expr::gte(lhs, rhs); break;

		case Token::Exp:
		{
			auto lhsLit = dynamic_pointer_cast<bg::IntLit const>(lhs);
			auto rhsLit = dynamic_pointer_cast<bg::IntLit const>(rhs);
			if (lhsLit && rhsLit)
				result = bg::Expr::intlit(boost::multiprecision::pow(lhsLit->getVal(), rhsLit->getVal().convert_to<unsigned>()));
			else
			{
				context.reportError(associatedNode, "Exponentiation is not supported in 'int' encoding");
				result = bg::Expr::error();
			}
			break;
		}
		default:
			context.reportError(associatedNode, string("Unsupported binary operator in 'int' encoding ") + TokenTraits::toString(op));
			result = bg::Expr::error();
		}
		break;
	case BoogieContext::Encoding::BV:
	{
		string name;
		string retType;

		switch (op)
		{
		case Token::Add:
		case Token::AssignAdd:
			result = context.bvAdd(bits, lhs, rhs); break;
		case Token::Sub:
		case Token::AssignSub:
			result = context.bvSub(bits, lhs, rhs); break;
		case Token::Mul:
		case Token::AssignMul:
			result = context.bvMul(bits, lhs, rhs); break;
		case Token::Div:
		case Token::AssignDiv:
			if (isSigned)
				result = context.bvSDiv(bits, lhs, rhs);
			else
				result = context.bvUDiv(bits, lhs, rhs);
			break;

		case Token::BitAnd:
		case Token::AssignBitAnd:
			result = context.bvAnd(bits, lhs, rhs); break;
		case Token::BitOr:
		case Token::AssignBitOr:
			result = context.bvOr(bits, lhs, rhs); break;
		case Token::BitXor:
		case Token::AssignBitXor:
			result = context.bvXor(bits, lhs, rhs); break;
		case Token::SAR:
		case Token::AssignSar:
			if (isSigned)
				result = context.bvAShr(bits, lhs, rhs);
			else
				result = context.bvLShr(bits, lhs, rhs);
			break;
		case Token::SHL:
		case Token::AssignShl:
			result = context.bvShl(bits, lhs, rhs); break;

		case Token::LessThan:
			if (isSigned)
				result = context.bvSlt(bits, lhs, rhs);
			else
				result = context.bvUlt(bits, lhs, rhs);
			break;
		case Token::GreaterThan:
			if (isSigned)
				result = context.bvSgt(bits, lhs, rhs);
			else
				result = context.bvUgt(bits, lhs, rhs);
			break;
		case Token::LessThanOrEqual:
			if (isSigned)
				result = context.bvSle(bits, lhs, rhs);
			else
				result = context.bvUle(bits, lhs, rhs);
			break;
		case Token::GreaterThanOrEqual:
			if (isSigned)
				result = context.bvSge(bits, lhs, rhs);
			else
				result = context.bvUge(bits, lhs, rhs);
			break;
		case Token::Exp:
		{
			auto lhsLit = dynamic_pointer_cast<bg::BvLit const>(lhs);
			auto rhsLit = dynamic_pointer_cast<bg::BvLit const>(rhs);
			if (lhsLit && rhsLit)
			{
				auto power = boost::multiprecision::pow(bg::bigint(lhsLit->getVal()),
						bg::bigint(rhsLit->getVal()).convert_to<unsigned>());
				result = context.intLit(power % boost::multiprecision::pow(bg::bigint(2), bits), bits);
			}
			else
			{
				context.reportError(associatedNode, "Exponentiation is not supported in 'bv' encoding");
				result = bg::Expr::error();
			}
			break;
		}
		default:
			context.reportError(associatedNode, string("Unsupported binary operator in 'bv' encoding ") + TokenTraits::toString(op));
			result = bg::Expr::error();
		}
		break;
	}
	case BoogieContext::Encoding::MOD:
	{
		auto modulo = bg::Expr::intlit(boost::multiprecision::pow(bg::bigint(2), bits));
		auto largestSigned = bg::Expr::intlit(boost::multiprecision::pow(bg::bigint(2), bits - 1) - 1);
		auto smallestSigned = bg::Expr::intlit(-boost::multiprecision::pow(bg::bigint(2), bits - 1));
		switch (op)
		{
		case Token::Add:
		case Token::AssignAdd:
		{
			auto sum = bg::Expr::plus(lhs, rhs);
			if (isSigned)
				result = bg::Expr::cond(bg::Expr::gt(sum, largestSigned),
					bg::Expr::minus(sum, modulo),
					bg::Expr::cond(bg::Expr::lt(sum, smallestSigned), bg::Expr::plus(sum, modulo), sum));
			else
				result = bg::Expr::cond(bg::Expr::gte(sum, modulo), bg::Expr::minus(sum, modulo), sum);
			ecc = bg::Expr::eq(sum, result);
			break;
		}
		case Token::Sub:
		case Token::AssignSub:
		{
			auto diff = bg::Expr::minus(lhs, rhs);
			if (isSigned)
				result = bg::Expr::cond(bg::Expr::gt(diff, largestSigned),
					bg::Expr::minus(diff, modulo),
					bg::Expr::cond(bg::Expr::lt(diff, smallestSigned), bg::Expr::plus(diff, modulo), diff));
			else
				result = bg::Expr::cond(bg::Expr::gte(lhs, rhs), diff, bg::Expr::plus(diff, modulo));
			ecc = bg::Expr::eq(diff, result);
			break;
		}
		case Token::Mul:
		case Token::AssignMul:
		{
			auto prod = bg::Expr::times(lhs, rhs);
			if (isSigned)
			{
				auto lhs1 = bg::Expr::cond(bg::Expr::gte(lhs, bg::Expr::intlit((long)0)), lhs, bg::Expr::plus(modulo, lhs));
				auto rhs1 = bg::Expr::cond(bg::Expr::gte(rhs, bg::Expr::intlit((long)0)), rhs, bg::Expr::plus(modulo, rhs));
				auto prod = bg::Expr::mod(bg::Expr::times(lhs1, rhs1), modulo);
				result = bg::Expr::cond(bg::Expr::gt(prod, largestSigned), bg::Expr::minus(prod, modulo), prod);
			}
			else
			{
				result = bg::Expr::cond(bg::Expr::gte(prod, modulo), bg::Expr::mod(prod, modulo), prod);
			}
			ecc = bg::Expr::eq(prod, result);
			break;
		}
		case Token::Div:
		case Token::AssignDiv:
		{
			auto div = bg::Expr::intdiv(lhs, rhs);
			if (isSigned)
				result = bg::Expr::cond(bg::Expr::gt(div, largestSigned),
					bg::Expr::minus(div, modulo),
					bg::Expr::cond(bg::Expr::lt(div, smallestSigned), bg::Expr::plus(div, modulo), div));
			else
				result = div;
			ecc = bg::Expr::eq(div, result);
			break;
		}

		case Token::LessThan:
			result = bg::Expr::lt(lhs, rhs);
			break;
		case Token::GreaterThan:
			result = bg::Expr::gt(lhs, rhs);
			break;
		case Token::LessThanOrEqual:
			result = bg::Expr::lte(lhs, rhs);
			break;
		case Token::GreaterThanOrEqual:
			result = bg::Expr::gte(lhs, rhs);
			break;
		case Token::Exp:
		{
			auto lhsLit = dynamic_pointer_cast<bg::IntLit const>(lhs);
			auto rhsLit = dynamic_pointer_cast<bg::IntLit const>(rhs);
			if (lhsLit && rhsLit)
			{
				auto power = boost::multiprecision::pow(lhsLit->getVal(),rhsLit->getVal().convert_to<unsigned>());
				result = context.intLit(power % boost::multiprecision::pow(bg::bigint(2), isSigned ? bits - 1 : bits), bits);
				ecc = bg::Expr::eq(context.intLit(power, bits), result);
			}
			else
			{
				context.reportError(associatedNode, "Exponentiation is not supported in 'mod' encoding");
				result = bg::Expr::error();
			}
			break;
		}
		default:
			context.reportError(associatedNode, string("Unsupported binary operator in 'mod' encoding ") + TokenTraits::toString(op));
			result = bg::Expr::error();
		}
		break;
	}
	default:
		solAssert(false, "Unknown arithmetic encoding");
	}

	assert(result != nullptr);
	return ExprWithCC{result, ecc};
}

ASTBoogieUtils::ExprWithCC ASTBoogieUtils::encodeArithUnaryOp(BoogieContext& context, ASTNode const* associatedNode, Token op,
		bg::Expr::Ref subExpr, unsigned bits, bool isSigned)
{
	bg::Expr::Ref result = nullptr;
	bg::Expr::Ref ecc = nullptr;

	switch (context.encoding())
	{
	case BoogieContext::Encoding::INT:
		switch (op)
		{
		case Token::Sub: result = bg::Expr::neg(subExpr); break;
		default:
			context.reportError(associatedNode, string("Unsupported unary operator in 'int' encoding ") + TokenTraits::toString(op));
			result = bg::Expr::error();
			break;
		}
		break;

	case BoogieContext::Encoding::BV:
		{
			string name("");
			switch (op)
			{
			case Token::Sub: result = context.bvNeg(bits, subExpr); break;
			case Token::BitNot: result = context.bvNot(bits, subExpr); break;
			default:
				context.reportError(associatedNode, string("Unsupported unary operator in 'bv' encoding ") + TokenTraits::toString(op));
				result = bg::Expr::error();
				break;
			}
		}
		break;

	case BoogieContext::Encoding::MOD:
		switch (op)
		{
		case Token::Sub:
		{
			auto sub = bg::Expr::neg(subExpr);
			if (isSigned)
			{
				auto smallestSigned = bg::Expr::intlit(-boost::multiprecision::pow(bg::bigint(2), bits - 1));
				result = bg::Expr::cond(bg::Expr::eq(subExpr, smallestSigned),
						smallestSigned,
						sub);
			}
			else
			{
				auto modulo = bg::Expr::intlit(boost::multiprecision::pow(bg::bigint(2), bits));
				result = bg::Expr::cond(bg::Expr::eq(subExpr, bg::Expr::intlit((long)0)),
						bg::Expr::intlit((long)0),
						bg::Expr::minus(modulo, subExpr));
			}
			ecc = bg::Expr::eq(sub, result);
			break;
		}
		default:
			context.reportError(associatedNode, string("Unsupported unary operator in 'mod' encoding ") + TokenTraits::toString(op));
			result = bg::Expr::error();
			break;
		}
		break;

	default:
		solAssert(false, "Unknown arithmetic encoding");
	}
	assert(result != nullptr);
	return ExprWithCC{result, ecc};
}

bool ASTBoogieUtils::isBitPreciseType(TypePointer type)
{
	switch (type->category())
	{
	case Type::Category::Integer:
	case Type::Category::FixedBytes:
	case Type::Category::Enum:
		return true;
	default:
		return false;
	}
	return true;
}

unsigned ASTBoogieUtils::getBits(TypePointer type)
{
	if (auto intType = dynamic_cast<IntegerType const*>(type))
		return intType->numBits();
	if (dynamic_cast<EnumType const*>(type))
		return 256;
	if (auto fb = dynamic_cast<FixedBytesType const*>(type))
		return fb->numBytes()*8;
	if (dynamic_cast<AddressType const*>(type))
		return 256;

	solAssert(false, "Trying to get bits for non-bitprecise type");
	return 0;
}

bool ASTBoogieUtils::isSigned(TypePointer type)
{
	if (auto intType = dynamic_cast<IntegerType const*>(type))
		return intType->isSigned();
	if (dynamic_cast<EnumType const*>(type))
		return false;
	if (dynamic_cast<FixedBytesType const*>(type))
		return false;
	if (dynamic_cast<AddressType const*>(type))
		return false;

	solAssert(false, "Trying to get sign for non-bitprecise type");
	return false;
}

bg::Expr::Ref ASTBoogieUtils::checkImplicitBvConversion(bg::Expr::Ref expr, TypePointer exprType, TypePointer targetType, BoogieContext& context)
{
	solAssert(exprType, "");
	solAssert(targetType, "");

	if (isBitPreciseType(targetType))
	{
		unsigned targetBits = getBits(targetType);
		// Create bitvector from literals
		if (auto exprLit = dynamic_pointer_cast<bg::IntLit const>(expr))
		{
			if (exprLit->getVal() < 0) // Negative literals are tricky
				return context.bvNeg(targetBits, bg::Expr::bvlit(-exprLit->getVal(), targetBits));
			else
				return bg::Expr::bvlit(exprLit->getVal(), targetBits);
		}
		else if (isBitPreciseType(exprType))
		{
			unsigned exprBits = getBits(exprType);
			bool targetSigned = isSigned(targetType);
			bool exprSigned = isSigned(exprType);

			// Nothing to do if size and signedness is the same
			if (targetBits == exprBits && targetSigned == exprSigned)
				return expr;
			// Conversion to smaller type should have already been detected by the compiler
			solAssert (targetBits >= exprBits, "Implicit conversion to smaller type");

			if (!exprSigned) // Unsigned can be converted to larger (signed or unsigned) with zero extension
				return context.bvZeroExt(expr, exprBits, targetBits);
			else if (targetSigned) // Signed can only be converted to signed with sign extension
				return context.bvSignExt(expr, exprBits, targetBits);
			else // Signed to unsigned should have already been detected by the compiler
			{
				solAssert(false, "Implicit conversion from signed to unsigned");
				return nullptr;
			}
		}
	}

	return expr;
}

bg::Expr::Ref ASTBoogieUtils::checkExplicitBvConversion(bg::Expr::Ref expr, TypePointer exprType, TypePointer targetType, BoogieContext& context)
{
	// Do nothing if any of the types is unknown
	if (!targetType || !exprType)
		return expr;

	if (isBitPreciseType(targetType))
	{
		unsigned targetBits = getBits(targetType);
		// Literals can be handled by implicit conversion
		if (dynamic_pointer_cast<bg::IntLit const>(expr))
		{
			return checkImplicitBvConversion(expr, exprType, targetType, context);
		}
		else if (isBitPreciseType(exprType))
		{
			unsigned exprBits = getBits(exprType);
			bool targetSigned = isSigned(targetType);
			bool exprSigned = isSigned(exprType);

			// Check if explicit conversion is really needed:
			// - converting to smaller size
			// - converting from signed to unsigned
			// - converting from unsigned to same size signed
			if (targetBits < exprBits || (exprSigned && !targetSigned) || (targetBits == exprBits && !exprSigned && targetSigned))
			{
				// Nothing to do for same size, since Boogie bitvectors do not have signs
				if (targetBits == exprBits)
					return expr;
				// For larger sizes, sign extension is done
				else if (targetBits > exprBits)
					return context.bvSignExt(expr, exprBits, targetBits);
				// For smaller sizes, higher-order bits are discarded
				else
					return context.bvExtract(expr, exprBits,targetBits-1, 0);
			}
			// Otherwise the implicit will handle it
			else
			{
				return checkImplicitBvConversion(expr, exprType, targetType, context);
			}
		}
	}

	return expr;
}

bg::Expr::Ref ASTBoogieUtils::getTCCforExpr(bg::Expr::Ref expr, TypePointer tp, ASTBoogieUtils::TCCType type)
{
	bg::Expr::Ref upperBound = nullptr;
	bg::Expr::Ref lowerBound = nullptr;

	// For enums we know the number of members
	if (tp->category() == Type::Category::Enum)
	{
		auto enumTp = dynamic_cast<EnumType const *>(tp);
		solAssert(enumTp, "");
		lowerBound = bg::Expr::lte(bg::Expr::intlit((long)0), expr);
		upperBound = bg::Expr::lt(expr, bg::Expr::intlit((unsigned long)enumTp->enumDefinition().members().size()));
	}
	// Otherwise get smallest and largest
	else if (isBitPreciseType(tp))
	{
		unsigned bits = getBits(tp);
		if (isSigned(tp))
		{
			auto largestSigned = bg::Expr::intlit(boost::multiprecision::pow(bg::bigint(2), bits - 1) - 1);
			auto smallestSigned = bg::Expr::intlit(-boost::multiprecision::pow(bg::bigint(2), bits - 1));
			lowerBound = bg::Expr::lte(smallestSigned, expr);
			upperBound = bg::Expr::lte(expr, largestSigned);
		}
		else
		{
			auto largestUnsigned = bg::Expr::intlit(boost::multiprecision::pow(bg::bigint(2), bits) - 1);
			auto smallestUnsigned = bg::Expr::intlit(long(0));
			lowerBound = bg::Expr::lte(smallestUnsigned, expr);
			upperBound = bg::Expr::lte(expr, largestUnsigned);
		}
	}

	// Return the TCC
	switch (type) {
	case LowerBound:
		return lowerBound;
	case UpperBound:
		return upperBound;
	case BothBounds:
		return bg::Expr::and_(lowerBound, upperBound);
	default:
		solAssert(false, "");
	}
	return bg::Expr::true_();
}

bg::Expr::Ref ASTBoogieUtils::defaultValue(TypePointer type, BoogieContext& context)
{
	return defaultValueInternal(type, context).bgExpr;
}

bg::Expr::Ref ASTBoogieUtils::stringValue(BoogieContext& context, std::string literal)
{
	auto keyType = context.intType(256);
	auto valType = context.intType(8);
	auto baseDefVal = defaultValueInternal(TypeProvider::byte(), context);

	string smtType = "(Array " + keyType->getSmtType() + " " + valType->getSmtType() + ")";
	string smtExpr = "((as const " + smtType + ") " + baseDefVal.smt + ")";

	bg::FuncDeclRef arrConstr = context.getArrayConstructor(valType);
	auto arrLength = context.intLit(literal.size(), 256);

	bg::FuncDeclRef arrayValue0 = context.defaultArray(keyType, valType, smtExpr);
	std::string arrayValue0Name = arrayValue0->getName();

	auto innerArr = bg::Expr::fn(arrayValue0Name, vector<bg::Expr::Ref>());
	for (size_t i = 0; i < literal.size(); ++ i)
	{
		auto iExpr = context.intLit(i, 256);
		auto cExpr = context.intLit(literal[i], 8);
		innerArr = bg::Expr::arrupd(innerArr, iExpr, cExpr);
	}

	// Construct the array with array setup
	auto bgExpr = bg::Expr::fn(arrConstr->getName(), {innerArr, arrLength});

	return bgExpr;
}

ASTBoogieUtils::Value ASTBoogieUtils::defaultValueInternal(TypePointer type, BoogieContext& context)
{
	switch (type->category()) // TODO: bitvectors
	{
	case Type::Category::Integer:
	{
		// 0
		auto lit = context.intLit(0, ASTBoogieUtils::getBits(type));
		return {lit->toSMT2String(), lit};
	}
	case Type::Category::Address:
	case Type::Category::Contract:
	case Type::Category::Enum:
	{
		// 0
		auto lit = context.intLit(0, 256);
		return {lit->toSMT2String(), lit};
	}
	case Type::Category::Bool:
	{
		// False
		auto lit = bg::Expr::false_();
		return {lit->toSMT2String(), lit};
	}
	case Type::Category::FixedBytes:
	{
		auto fbType = dynamic_cast<FixedBytesType const*>(type);
		auto lit = context.intLit(0, fbType->numBytes() * 8);
		return {lit->toSMT2String(), lit};
	}
	case Type::Category::Struct:
	{
		// default for all members
		StructType const* structType = dynamic_cast<StructType const*>(type);
		if (structType->dataStoredIn(DataLocation::Storage))
		{
			MemberList const& members = structType->members(0); // No need for scope, just regular struct members
			// Get the default value for the members
			vector<bg::Expr::Ref> memberDefValExprs;
			vector<string> memberDefValSmts;
			for (auto& member: members)
			{
				Value memberDefaultValue = defaultValueInternal(member.type, context);
				if (memberDefaultValue.bgExpr == nullptr)
					return {"", nullptr};
				memberDefValExprs.push_back(memberDefaultValue.bgExpr);
				memberDefValSmts.push_back(memberDefaultValue.smt);
			}
			// Now construct the struct value
			StructDefinition const& structDefinition = structType->structDefinition();
			bg::FuncDeclRef structConstr = context.getStructConstructor(&structDefinition);
			string smtExpr = "(|" + structConstr->getName() + "|";
			for (string s: memberDefValSmts)
				smtExpr += " " + s;
			smtExpr += ")";
			return {smtExpr, bg::Expr::fn(structConstr->getName(), memberDefValExprs)};
		}
		break;
	}
	case Type::Category::Array:
	{
		ArrayType const* arrayType = dynamic_cast<ArrayType const*>(type);
		if (arrayType->dataStoredIn(DataLocation::Storage))
		{
			auto keyType = context.intType(256);
			auto valType = context.toBoogieType(arrayType->baseType(), nullptr);
			auto baseDefVal = defaultValueInternal(arrayType->baseType(), context);
			if (!baseDefVal.bgExpr)
				return {"", nullptr};
			string smtType = "(Array " + keyType->getSmtType() + " " + valType->getSmtType() + ")";
			string smtExpr = "((as const " + smtType + ") " + baseDefVal.smt + ")";
			bg::FuncDeclRef arrConstr = context.getArrayConstructor(valType);
			auto arrLength = context.intLit(arrayType->length(), 256);
			auto innerArr = bg::Expr::fn(context.defaultArray(keyType, valType, smtExpr)->getName(), vector<bg::Expr::Ref>());
			vector<bg::Expr::Ref> args;
			args.push_back(innerArr);
			args.push_back(arrLength);
			auto bgExpr = bg::Expr::fn(arrConstr->getName(), args);
			// SMT expression is constructed afterwards (not to be included in the const array function)
			smtExpr = "(|" + arrConstr->getName() + "| " + smtExpr + " " + arrLength->toBgString() + ")";
			return {smtExpr, bgExpr};
		}
		break;
	}
	case Type::Category::Mapping:
	{
		MappingType const* mappingType = dynamic_cast<MappingType const*>(type);
		auto mappingTypeKey = mappingType->keyType();
		if (mappingType->dataStoredIn(DataLocation::Storage))
			mappingTypeKey = TypeProvider::withLocationIfReference(DataLocation::Storage, mappingTypeKey);
		auto keyType = context.toBoogieType(mappingTypeKey, nullptr);
		auto valType = context.toBoogieType(mappingType->valueType(), nullptr);
		auto mapType = context.toBoogieType(type, nullptr);

		auto baseDefVal = defaultValueInternal(mappingType->valueType(), context);
		string smtType = mapType->getSmtType();
		string smtExpr = "((as const " + smtType + ") " + baseDefVal.smt + ")";
		auto bgExpr = bg::Expr::fn(context.defaultArray(keyType, valType, smtExpr)->getName(), vector<bg::Expr::Ref>());
		return {smtExpr, bgExpr};
	}
	default:
		// For unhandled, just return null
		break;
	}

	return {"", nullptr};
}

ASTBoogieUtils::AllocResult ASTBoogieUtils::newStruct(StructDefinition const* structDef, BoogieContext& context)
{
	// Address of the new struct
	string prefix = "new_struct_" + structDef->name();
	bg::TypeDeclRef varType = context.getStructType(structDef, DataLocation::Memory);
	bg::VarDeclRef tmpVar = context.freshTempVar(varType, prefix);
	return AllocResult{tmpVar, {
			bg::Stmt::assign(tmpVar->getRefTo(), context.getAllocCounter()->getRefTo()),
			context.incrAllocCounter()
	}};
}

ASTBoogieUtils::AllocResult ASTBoogieUtils::newArray(bg::TypeDeclRef type, BoogieContext& context)
{
	bg::VarDeclRef tmpVar =  context.freshTempVar(type, "new_array");
	return AllocResult{tmpVar, {
			bg::Stmt::assign(tmpVar->getRefTo(), context.getAllocCounter()->getRefTo()),
			context.incrAllocCounter()
	}};
}

}
}
