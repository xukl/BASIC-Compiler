#ifndef STATEMENT_HPP
#define STATEMENT_HPP
#include "expr.hpp"
#include <istream>
#include <ostream>
#include <map>
#include <vector>
#include <climits>

namespace statement
{
using line_num = int;
struct statement
{
	virtual ~statement() {}
	virtual void print(std::ostream &) const = 0;
	virtual std::unique_ptr<statement> deep_copy() const = 0;
};
std::ostream &operator<< (std::ostream &os, const statement &x);
struct REM : statement
{
	void print(std::ostream &os) const
	{
		os << "{rem}";
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<REM>();
	}
};
struct assignment
{
	std::unique_ptr<expr::expr> var;
	std::unique_ptr<expr::expr> val;
	assignment(std::unique_ptr<expr::expr> &&_var,
			std::unique_ptr<expr::expr> &&_val)
		: var(std::move(_var)), val(std::move(_val)) {}
	assignment(assignment &&) = default;
	assignment &operator= (assignment &&) = default;
	assignment(const std::string &str)
	{
		auto equal_sign_pos = str.find('=');
		var = expr::parse_expr(str.substr(0, equal_sign_pos));
		val = expr::parse_expr(str.substr(equal_sign_pos + 1));
	}
	void print(std::ostream &os) const
	{
		os << "{assign " << *var << " <- " << *val << '}';
	}
	assignment copy() const
	{
		return assignment(var->deep_copy(), val->deep_copy());
	}
};
std::ostream &operator<< (std::ostream &os, const assignment &x);
struct LET : statement
{
	assignment assign;
	LET(assignment &&_assign) : assign(std::move(_assign)) {}
	LET(const std::string &str) : assign(str) {}
	void print(std::ostream &os) const
	{
		assign.print(os);
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<LET>(assign.copy());
	}
};
struct INPUT : statement
{
	std::vector<std::unique_ptr<expr::expr>> inputs;
	INPUT(std::vector<std::unique_ptr<expr::expr>> &&_inputs)
		: inputs(std::move(_inputs)) {}
	INPUT(const std::string &str)
	{
		for (size_t pos = 0, nxt_pos = str.find(',');
				pos != str.npos;
				pos = nxt_pos, nxt_pos = str.find(',', pos + 1))
			inputs.push_back
				(expr::parse_expr(str.substr(pos + 1, nxt_pos - pos - 1)));
	}
	void print(std::ostream &os) const
	{
		os << "{input";
		for (auto &&x : inputs)
			os << ' ' << *x;
		os << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		decltype(inputs) tmp_inputs;
		for (const auto &e : inputs)
			tmp_inputs.push_back(e->deep_copy());
		return std::make_unique<INPUT>(std::move(tmp_inputs));
	}
};
struct EXIT : statement
{
	std::unique_ptr<expr::expr> val;
	EXIT(std::unique_ptr<expr::expr> &&_val) : val(std::move(_val)) {}
	EXIT(const std::string &str) : val(expr::parse_expr(str)) {}
	void print(std::ostream &os) const
	{
		os << "{exit " << *val << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<EXIT>(val->deep_copy());
	}
};
struct GOTO : statement
{
	line_num line;
	GOTO(const line_num &_line) : line(_line) {}
	GOTO(const std::string &str) : line(expr::parse_unsigned_num(str)) {}
	void print(std::ostream &os) const
	{
		os << "{goto " << line << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<GOTO>(line);
	}
};
struct IF : statement
{
	std::unique_ptr<expr::expr> condition;
	line_num line;
	IF(std::unique_ptr<expr::expr> &&_cond, const line_num &_line)
		: condition(std::move(_cond)), line(_line) {}
	IF(const std::string &str)
	{
		auto then_pos = str.find("THEN");
		std::string expr_str(str, 0, then_pos);
		std::string line_str(str, then_pos + 4);
		condition = expr::parse_expr(expr_str);
		line = expr::parse_unsigned_num(line_str);
	}
	void print(std::ostream &os) const
	{
		os << "{if_goto " << *condition << ' ' << line << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<IF>(condition->deep_copy(), line);
	}
};
struct FOR : statement
{
	std::unique_ptr<expr::expr> condition;
	line_num end_for_line;
	FOR(std::unique_ptr<expr::expr> &&_cond, const line_num &_end_for_line)
		: condition(std::move(_cond)), end_for_line(_end_for_line) {}
	FOR(const std::string &str, const line_num &_end_for_line)
		: condition(expr::parse_expr(std::string(str, str.find(';') + 1))),
		end_for_line(_end_for_line)
	{}
	void print(std::ostream &os) const
	{
		os << "{for while (" << *condition << "),"
			" end_for at line " << end_for_line << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<FOR>(condition->deep_copy(), end_for_line);
	}
};
struct END_FOR : statement
{
	line_num for_line;
	assignment step_statement;
	END_FOR(const line_num &_for_line, assignment &&step)
		: for_line(_for_line), step_statement(std::move(step)) {}
	void print(std::ostream &os) const
	{
		os << "{end_for of {for} at line " << for_line
			<< ", step_statement " << step_statement << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<END_FOR>(for_line, step_statement.copy());
	}
};
using program_type = std::map<line_num, std::unique_ptr<statement>>;
const int additional_exit_line = INT_MAX;
program_type read_program(std::istream &is);
void print_program(std::ostream &os, const program_type &prog);
}
#endif
