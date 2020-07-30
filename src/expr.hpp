#ifndef EXPR_HPP
#define EXPR_HPP
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
	virtual std::unique_ptr<expr> deep_copy() const = 0;
};
std::ostream &operator<< (std::ostream &os, const expr &x);
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
	std::unique_ptr<expr> deep_copy() const
	{
		return std::make_unique<id>(id_name);
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
	std::unique_ptr<expr> deep_copy() const
	{
		return std::make_unique<imm_num>(value);
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
	std::unique_ptr<expr> deep_copy() const\
	{\
		return std::make_unique<name>(lc->deep_copy(), rc->deep_copy());\
	}\
};
STRUCT_BIN(add)
STRUCT_BIN(sub)
STRUCT_BIN(mul)
STRUCT_BIN(div)
STRUCT_BIN(subscript)
#undef STRUCT_BIN
struct neg : unary_op
{
	using unary_op::unary_op;
	const char *op_name() const
	{
		return "neg";
	}
	std::unique_ptr<expr> deep_copy() const
	{
		return std::make_unique<neg>(c->deep_copy());
	}
};
struct cmp : bin_op
{
	const enum cmp_op { LT, LE, GT, GE, EQ, NE } op;
	cmp(cmp_op _op, std::unique_ptr<expr> &&_l, std::unique_ptr<expr> &&_r)
		: bin_op::bin_op(std::move(_l), std::move(_r)), op(_op) {}
	const char *op_name() const
	{
		switch (op)
		{
#define op_case(CMP_OP, print)\
			case CMP_OP:\
				return print;
			op_case(LT, "<")
			op_case(LE, "<=")
			op_case(GT, ">")
			op_case(GE, ">=")
			op_case(EQ, "==")
			op_case(NE, "!=")
#undef op_case
			default:
				throw "Invalid cmp_op.";
		}
	}
	std::unique_ptr<expr> deep_copy() const
	{
		return std::make_unique<cmp>(op, lc->deep_copy(), rc->deep_copy());
	}
};
struct bool_and : bin_op
{
	using bin_op::bin_op;
	const char *op_name() const
	{
		return "and";
	}
	std::unique_ptr<expr> deep_copy() const
	{
		return std::make_unique<bool_and>(lc->deep_copy(), rc->deep_copy());
	}
};
struct bool_or : bin_op
{
	using bin_op::bin_op;
	const char *op_name() const
	{
		return "or";
	}
	std::unique_ptr<expr> deep_copy() const
	{
		return std::make_unique<bool_or>(lc->deep_copy(), rc->deep_copy());
	}
};

id parse_id(const std::string &id_str);
value_type parse_unsigned_num(const std::string &num_str);
std::unique_ptr<expr> parse_expr(const std::string &expr_str);
}
#endif
