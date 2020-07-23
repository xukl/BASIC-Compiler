#include <ostream>
#include <string>
#include <utility>
#include <memory>

namespace expr
{
using expr_ite = std::string::const_iterator;
using value_type = int;
struct expr
{
	virtual ~expr() {}
	virtual void print(std::ostream &) const = 0;
};
std::ostream &operator<< (std::ostream &os, const expr &x)
{
	x.print(os);
	return os;
}
struct bin_op : expr
{
	virtual const char *op_name() const = 0;
	std::unique_ptr<expr> lc, rc;
	bin_op(std::unique_ptr<expr> &&_l, std::unique_ptr<expr> &&_r)
		: lc(std::move(_l)), rc(std::move(_r)) {}
	void print(std::ostream &os) const
	{
		os << '{' << op_name() << ' ' << *lc << ' ' << *rc << '}';
	}
};
struct unary_op : expr
{
	virtual const char *op_name() const = 0;
	std::unique_ptr<expr> c;
	unary_op(std::unique_ptr<expr> &&_c) : c(std::move(_c)) {}
	void print(std::ostream &os) const
	{
		os << '{' << op_name() << ' ' << *c << '}';
	}
};
struct id : expr
{
	std::string id_name;
	id(std::string &&_id_name) : id_name(std::move(_id_name)) {}
	id(const std::string &_id_name) : id_name(_id_name) {}
	void print(std::ostream &os) const
	{
		os << "{var " << id_name << '}';
	}
};
struct imm_num : expr
{
	value_type value;
	imm_num(const value_type &v) : value(v) {}
	void print(std::ostream &os) const
	{
		os << "{imm " << value << '}';
	}
};
#define STRUCT_BIN(name)\
struct name : bin_op\
{\
	using bin_op::bin_op;\
	const char *op_name() const\
	{\
		return #name;\
	}\
};
STRUCT_BIN(add)
STRUCT_BIN(sub)
STRUCT_BIN(mul)
STRUCT_BIN(div)
struct subscript : expr
{
	std::unique_ptr<expr> arr;
	std::unique_ptr<expr> idx;
	subscript(std::unique_ptr<expr> &&_arr, std::unique_ptr<expr> &&_idx)
		: arr(std::move(_arr)), idx(std::move(_idx)) {}
	void print(std::ostream &os) const
	{
		os << "{subscript " << *arr << ' ' << *idx << '}';
	}
};
struct neg : unary_op
{
	using unary_op::unary_op;
	const char *op_name() const
	{
		return "neg";
	}
};

using parse_expr_result = std::unique_ptr<expr>;
using parse_id_result = std::pair<std::unique_ptr<id>, expr_ite>;
inline void skip_space(expr_ite &ptr, const expr_ite &end)
{
	while (ptr < end && isspace(*ptr))
		++ptr;
}
/*
 * {E0} ::= ({expr}) | {id} | {IMM}
 * {E1} ::= {E0} | {id}[{expr}]
 * {E2} ::= {E1} | +{E2} | -{E2}
 * {E3} ::= {E2} | {E3} * {E2} | {E3} / {E2}
 * {expr} ::= {E3} | {expr} + {E3} | {expr} - {E3}
 */
std::unique_ptr<expr> parse_expr(expr_ite &first, const expr_ite &last);
id parse_id(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	auto name_begin = first;
	while (first < last && isalpha(*first))
		++first;
	return std::string(name_begin, first);
}
imm_num parse_unsigned_num(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	value_type value = 0;
	while (first < last && isdigit(*first))
	{
		value = value * 10 + *first - '0';
		++first;
	}
	return imm_num(value);
}
std::unique_ptr<expr> parse_e0(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "The expression is incomplete.";
	if (*first == '(')
	{
		++first;
		auto &&expr = parse_expr(first, last);
		skip_space(first, last);
		if (first < last && *first == ')')
		{
			++first;
			return std::move(expr);
		}
		throw ":-( expected ')'.";
	}
	else if (isdigit(*first))
		return std::make_unique<imm_num>(parse_unsigned_num(first, last));
	else if (isalpha(*first))
		return std::make_unique<id>(parse_id(first, last));
	else
		throw "Error at parse_e0. Probably caused by invalid character.";
}
std::unique_ptr<expr> parse_e1(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "The expression is incomplete.";
	auto &&ret = parse_e0(first, last);
	skip_space(first, last);
	while (first < last && *first == '[')
	{
		++first;
		auto &&idx = parse_expr(first, last);
		skip_space(first, last);
		if (first == last || *first != ']')
			throw ":-( unclosed '['.";
		++first;
		skip_space(first, last);
		ret = std::make_unique<subscript>(std::move(ret), std::move(idx));
	}
	return std::move(ret);
}
std::unique_ptr<expr> parse_e2(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "The expression is incomplete.";
	int sign = +1;
	while (*first == '+' || *first == '-')
	{
		if (*first == '-')
			sign = -sign;
		++first;
		skip_space(first, last);
	}
	auto &&e1 = parse_e1(first, last);
	if (sign == +1)
		return std::move(e1);
	return std::make_unique<neg>(std::move(e1));
}
std::unique_ptr<expr> parse_e3(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	auto &&ret = parse_e2(first, last);
	skip_space(first, last);
	while (first < last && (*first == '*' || *first == '/'))
	{
		char op_tmp = *first;
		++first;
		auto &&tmp = parse_e2(first, last);
		skip_space(first, last);
		if (op_tmp == '*')
			ret = std::make_unique<mul>(std::move(ret), std::move(tmp));
		else
			ret = std::make_unique<div>(std::move(ret), std::move(tmp));
	}
	return std::move(ret);
}
std::unique_ptr<expr> parse_expr(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	auto &&ret = parse_e3(first, last);
	skip_space(first, last);
	while (first < last && (*first == '+' || *first == '-'))
	{
		char op_tmp = *first;
		++first;
		auto &&tmp = parse_e3(first, last);
		skip_space(first, last);
		if (op_tmp == '+')
			ret = std::make_unique<add>(std::move(ret), std::move(tmp));
		else
			ret = std::make_unique<sub>(std::move(ret), std::move(tmp));
	}
	return std::move(ret);
}
std::unique_ptr<expr> parse_expr(const std::string &expr_str)
{
	auto first = expr_str.cbegin();
	const auto last = expr_str.cend();
	auto &&ret = parse_expr(first, last);
	skip_space(first, last);
	if (first != last)
		throw "Extra trailing characters.";
	return std::move(ret);
}

struct bool_expr
{
	virtual ~bool_expr() {}
	virtual void print(std::ostream &) const = 0;
};
std::ostream &operator<< (std::ostream &os, const bool_expr &x)
{
	x.print(os);
	return os;
}
struct cmp : bool_expr
{
	const enum cmp_op { LT, LE, GT, GE, EQ, NE } op;
	std::unique_ptr<expr> lc, rc;
	cmp(cmp_op _op, std::unique_ptr<expr> &&_l, std::unique_ptr<expr> &&_r)
		: op(_op), lc(std::move(_l)), rc(std::move(_r)) {}
	void print(std::ostream &os) const
	{
		os << '{';
		switch (op)
		{
#define op_case(CMP_OP, print)\
			case CMP_OP:\
				os << print;\
				break;
			op_case(LT, "<")
			op_case(LE, "<=")
			op_case(GT, ">")
			op_case(GE, ">=")
			op_case(EQ, "==")
			op_case(NE, "!=")
		}
		os << ' ' << *lc << ' ' << *rc << '}';
	}
};
struct bool_and : bool_expr
{
	std::unique_ptr<bool_expr> lc, rc;
	bool_and(std::unique_ptr<bool_expr> &&_l, std::unique_ptr<bool_expr> &&_r)
		: lc(std::move(_l)), rc(std::move(_r)) {}
	void print(std::ostream &os) const
	{
		os << "{and " << *lc << ' ' << *rc << '}';
	}
};
struct bool_or : bool_expr
{
	std::unique_ptr<bool_expr> lc, rc;
	bool_or(std::unique_ptr<bool_expr> &&_l, std::unique_ptr<bool_expr> &&_r)
		: lc(std::move(_l)), rc(std::move(_r)) {}
	void print(std::ostream &os) const
	{
		os << "{or " << *lc << ' ' << *rc << '}';
	}
};
/*
 * {B0} ::= {expr} @cmp_op {expr} | ({bool_expr})
 * {B1} ::= {B0} | {B1} && {B0}
 * {bool_expr} ::= {B1} | {B2} || {B1}
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
		first = ret.second;
		if (first < last && *first == ')')
			return parse_bool_result(std::move(ret.first), first + 1);
		throw ":-( expected ')'";
	}
	auto &&expr_1 = parse_expr(first, last);
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
	auto expr_2 = parse_expr(first, last);
	return parse_bool_result(std::make_unique<cmp>
			(op, std::move(expr_1), std::move(expr_2)),
			first);
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
		skip_space(first, last);
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
		skip_space(first, last);
		ret = parse_bool_result(std::make_unique<bool_or>
				(std::move(ret.first), std::move(tmp.first)),
				first);
	}
	return ret;
}
}
