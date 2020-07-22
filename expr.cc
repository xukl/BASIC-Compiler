#include <string>
#include <utility>
#include <memory>

namespace expr
{
using expr_ite = std::string::const_iterator;
using value_type = int;
struct expr
{
	virtual ~expr();
};
struct bin_op : expr
{
	std::unique_ptr<expr> lc, rc;
	bin_op(std::unique_ptr<expr> &&_l, std::unique_ptr<expr> &&_r)
		: lc(std::move(_l)), rc(std::move(_r)) {}
};
struct unary_op : expr
{
	std::unique_ptr<expr> c;
	unary_op(std::unique_ptr<expr> &&_c) : c(std::move(_c)) {}
};
struct id : expr
{
	std::string id_name;
	template <typename ...Args>
	id(Args&&... args) : id_name(std::forward<Args>(args)...) {}
};
struct imm_num : expr
{
	value_type value;
	imm_num(const value_type &v) : value(v) {}
};
#define STRUCT_BIN(name)\
struct name : bin_op\
{\
	using bin_op::bin_op;\
};
STRUCT_BIN(add)
STRUCT_BIN(sub)
STRUCT_BIN(mul)
STRUCT_BIN(div)
struct subscript : expr
{
	std::unique_ptr<id> arr;
	std::unique_ptr<expr> idx;
	subscript(std::unique_ptr<id> &&_arr, std::unique_ptr<expr> &&_idx)
		: arr(std::move(_arr)), idx(std::move(_idx)) {}
};
struct neg : unary_op
{
	using unary_op::unary_op;
};

using parse_expr_result = std::pair<std::unique_ptr<expr>, expr_ite>;
using parse_id_result = std::pair<std::unique_ptr<id>, expr_ite>;
inline void skip_space(expr_ite &ptr, const expr_ite &end)
{
	while (ptr < end && isspace(*ptr))
		++ptr;
}
/*
 * {E0} ::= ({expr}) | {id} | {IMM}
 * {E1} ::= {E0} | {id}[{expr}]
 * {E2} ::= {E1} | +{E1} | -{E1}
 * {E3} ::= {E2} | {E2} * {E2} | {E2} / {E2}
 * {expr} ::= {E3} | {E3} + {E3} | {E3} - {E3}
 */
parse_expr_result parse_expr(expr_ite first, const expr_ite &last);
std::pair<std::unique_ptr<id>, expr_ite>
parse_id(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	auto begin_name = first;
	while (first < last && isalpha(*first))
		++first;
	return std::pair<std::unique_ptr<id>, expr_ite>
		(std::make_unique<id>(begin_name, first), first);
}
std::pair<std::unique_ptr<imm_num>, expr_ite>
parse_unsigned_num(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	value_type value;
	while (first < last && isdigit(*first))
	{
		value = value * 10 + *first - '0';
		++first;
	}
	return std::pair<std::unique_ptr<imm_num>, expr_ite>
		(std::make_unique<imm_num>(value), first);
}
parse_expr_result parse_e0(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "The expression is incomplete.";
	if (*first == '(')
	{
		++first;
		auto &&tmp = parse_expr(first, last);
		first = tmp.second;
		skip_space(first, last);
		if (first < last && *first == ')')
			return parse_expr_result(std::move(tmp.first), first + 1);
		throw ":-( expected ')'.";
	}
	else if (isdigit(*first))
		return parse_unsigned_num(first, last);
	else if (isalpha(*first))
		return parse_id(first, last);
	else
		throw "Error at parse_e0. Probably caused by invalid character.";
}
parse_expr_result parse_e1(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "The expression is incomplete.";
	if (isalpha(*first))
	{
		auto &&id = parse_id(first, last);
		first = id.second;
		skip_space(first, last);
		if (first < last && *first == '[')
		{
			auto &&idx = parse_expr(first, last);
			first = idx.second;
			skip_space(first, last);
			if (first < last && *first == ']')
				return parse_expr_result(std::make_unique<subscript>
						(std::move(id.first), std::move(idx.first)), first + 1);
			throw ":-( unclosed '['.";
		}
		return parse_expr_result(std::move(id.first), first + 1);
	}
	return parse_e0(first, last);
}
parse_expr_result parse_e2(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "The expression is incomplete.";
	if (*first == '-')
	{
		++first;
		auto tmp = parse_e1(first, last);
		return parse_expr_result(std::make_unique<neg>(std::move(tmp.first)), tmp.second);
	}
	else
	{
		if (*first == '+')
			++first;
		return parse_e1(first, last);
	}
}
parse_expr_result parse_e3(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	auto a = parse_e2(first, last);
	first = a.second;
	skip_space(first, last);
	if (first == last)
		return parse_expr_result(std::move(a.first), first);
	if (*first == '*')
	{
		++first;
		auto b = parse_e2(first, last);
		return parse_expr_result
			(std::make_unique<mul>(std::move(a.first), std::move(b.first)),
			 b.second);
	}
	else if (*first == '/')
	{
		++first;
		auto b = parse_e2(first, last);
		return parse_expr_result
			(std::make_unique<div>(std::move(a.first), std::move(b.first)),
			 b.second);
	}
	else
		return parse_expr_result(std::move(a.first), first);
}
parse_expr_result parse_expr(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	auto a = parse_e3(first, last);
	first = a.second;
	skip_space(first, last);
	if (first == last)
		return parse_expr_result(std::move(a.first), first);
	if (*first == '+')
	{
		++first;
		auto b = parse_e3(first, last);
		return parse_expr_result
			(std::make_unique<add>(std::move(a.first), std::move(b.first)),
			 b.second);
	}
	else if (*first == '-')
	{
		++first;
		auto b = parse_e3(first, last);
		return parse_expr_result
			(std::make_unique<sub>(std::move(a.first), std::move(b.first)),
			 b.second);
	}
	else
		return parse_expr_result(std::move(a.first), first);
}

struct bool_expr
{
	virtual ~bool_expr() {}
};
struct cmp : bool_expr
{
	const enum cmp_op { LT, LE, GT, GE, EQ, NE } op;
	std::unique_ptr<expr> lc, rc;
	cmp(cmp_op _op, std::unique_ptr<expr> &&_l, std::unique_ptr<expr> &&_r)
		: op(_op), lc(std::move(_l)), rc(std::move(_r)) {}
};
struct bool_and : bool_expr
{
	std::unique_ptr<bool_expr> lc, rc;
	bool_and(std::unique_ptr<bool_expr> &&_l, std::unique_ptr<bool_expr> &&_r)
		: lc(std::move(_l)), rc(std::move(_r)) {}
};
struct bool_or : bool_expr
{
	std::unique_ptr<bool_expr> lc, rc;
	bool_or(std::unique_ptr<bool_expr> &&_l, std::unique_ptr<bool_expr> &&_r)
		: lc(std::move(_l)), rc(std::move(_r)) {}
};
/*
 * {B0} ::= {expr} @cmp_op {expr}
 * {B1} ::= {B0} | {B0} && {B0}
 * {bool_expr} ::= {B1} | {B1} || {B1}
 */
using parse_bool_result = std::pair<std::unique_ptr<bool_expr>, expr_ite>;
parse_bool_result parse_bool_expr(expr_ite first, const expr_ite &last);
parse_bool_result parse_b0(expr_ite first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "bool expression incomplete or missing.";
	if (*first == '(')
	{
		++first;
		auto ret = parse_bool_expr(first, last);
		if (first < last && *first == ')')
			return parse_bool_result(std::move(ret.first), first + 1);
		throw ":-( expected ')'";
	}
	auto tmp_1 = parse_expr(first, last);
	first = tmp_1.second;
	skip_space(first, last);
	if (first >= last - 1) // op and another {expr}, at least 2 chars
		throw "invalid bool expression.";
	cmp::cmp_op op;
	switch (*first)
	{
		case '!':
			if (*(first + 1) == '=')
			{
				op = cmp::NE;
				first += 2;
			}
			else
				throw "Invalid cmp op.";
			break;
		case '=':
			if (*(first + 1) == '=')
			{
				op = cmp::EQ;
				first += 2;
			}
			else
				throw "Invalid cmp op.";
			break;
		case '<':
			if (*(first + 1) == '=')
			{
				op = cmp::LE;
				first += 2;
			}
			else
			{
				op = cmp::LT;
				++first;
			}
			break;
		case '>':
			if (*(first + 1) == '=')
			{
				op = cmp::GE;
				first += 2;
			}
			else
			{
				op = cmp::GT;
				++first;
			}
			break;
		default:
			throw "Invalid cmp op.";
	}
	auto tmp_2 = parse_expr(first, last);
	return parse_bool_result(std::make_unique<cmp>
			(op, std::move(tmp_1.first), std::move(tmp_2.first)),
			tmp_2.second);
}
parse_bool_result parse_b1(expr_ite first, const expr_ite &last)
{
	auto ret = parse_b0(first, last);
	first = ret.second;
	skip_space(first, last);
	while ((first <= last - 2) && *first == '&' && *(first + 1) == '&')
	{
		auto tmp = parse_b0(first, last);
		first = tmp.second;
		ret = parse_bool_result(std::make_unique<bool_and>
				(std::move(ret.first), std::move(tmp.first)),
				first);
	}
	return ret;
}
parse_bool_result parse_bool_expr(expr_ite first, const expr_ite &last)
{
	auto ret = parse_b1(first, last);
	first = ret.second;
	skip_space(first, last);
	while ((first <= last - 2) && *first == '|' && *(first + 1) == '|')
	{
		auto tmp = parse_b1(first, last);
		first = tmp.second;
		ret = parse_bool_result(std::make_unique<bool_or>
				(std::move(ret.first), std::move(tmp.first)),
				first);
	}
	return ret;
}
}
