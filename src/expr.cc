#include "expr.hpp"

namespace expr
{
inline void skip_space(expr_ite &ptr, const expr_ite &end)
{
	while (ptr < end && isspace(*ptr))
		++ptr;
}
std::ostream &operator<< (std::ostream &os, const expr &x)
{
	x.print(os);
	return os;
}
/*
 * {E0} ::= ({expr}) | {id} | {IMM}
 * {E1} ::= {E0} | {E1}[{expr}]
 * {E2} ::= {E1} | +{E2} | -{E2}
 * {E3} ::= {E2} | {E3} * {E2} | {E3} / {E2}
 * {val_expr} ::= {E3} | {val_expr} + {E3} | {val_expr} - {E3}
 * {B0} ::= {val_expr} @cmp_op {val_expr} | ({expr})
 * {B1} ::= {B0} | {B1} && {B0}
 * {expr} ::= {B1} | {B2} || {B1}
 */
id parse_id(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	auto name_begin = first;
	while (first < last && isalpha(*first))
		++first;
	return std::string(name_begin, first);
}
id parse_id(const std::string &id_str)
{
	auto first = id_str.cbegin();
	const auto last = id_str.cend();
	auto &&ret = parse_id(first, last);
	skip_space(first, last);
	if (first != last)
		throw "Extra trailing characters.";
	return std::move(ret);
}
value_type parse_unsigned_num(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	value_type value = 0;
	while (first < last && isdigit(*first))
	{
		value = value * 10 + *first - '0';
		++first;
	}
	return value;
}
value_type parse_unsigned_num(const std::string &num_str)
{
	auto first = num_str.cbegin();
	const auto last = num_str.cend();
	auto &&ret = parse_unsigned_num(first, last);
	skip_space(first, last);
	if (first != last)
		throw "Extra trailing characters.";
	return std::move(ret);
}
std::unique_ptr<expr> parse_expr(expr_ite &first, const expr_ite &last);
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
			throw ":-( expected '['.";
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
std::unique_ptr<expr> parse_val_expr(expr_ite &first, const expr_ite &last)
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
std::unique_ptr<expr> parse_val_expr(const std::string &expr_str)
{
	auto first = expr_str.cbegin();
	const auto last = expr_str.cend();
	auto &&ret = parse_val_expr(first, last);
	skip_space(first, last);
	if (first != last)
		throw "Extra trailing characters.";
	return std::move(ret);
}

std::unique_ptr<expr> parse_b0(expr_ite &first, const expr_ite &last)
{
	skip_space(first, last);
	if (first == last)
		throw "bool expression incomplete or missing.";
	auto &&ret = parse_val_expr(first, last);
	skip_space(first, last);
	while (first <= last - 2) // op and another {expr}, at least 2 chars
	{
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
				return std::move(ret);
		}
		auto &&tmp = parse_val_expr(first, last);
		ret = std::make_unique<cmp>(op, std::move(ret), std::move(tmp));
	}
	return std::move(ret);
}
std::unique_ptr<expr> parse_b1(expr_ite &first, const expr_ite &last)
{
	auto &&ret = parse_b0(first, last);
	skip_space(first, last);
	while ((first <= last - 2) && *first == '&' && *(first + 1) == '&')
	{
		first += 2;
		auto &&tmp = parse_b0(first, last);
		skip_space(first, last);
		ret = std::make_unique<bool_and>(std::move(ret), std::move(tmp));
	}
	return std::move(ret);
}
std::unique_ptr<expr> parse_expr(expr_ite &first, const expr_ite &last)
{
	auto &&ret = parse_b1(first, last);
	skip_space(first, last);
	while ((first <= last - 2) && *first == '|' && *(first + 1) == '|')
	{
		first += 2;
		auto &&tmp = parse_b1(first, last);
		skip_space(first, last);
		ret = std::make_unique<bool_or>(std::move(ret), std::move(tmp));
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
}
