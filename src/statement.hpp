#ifndef STATEMENT_HPP
#define STATEMENT_HPP
#include "expr.hpp"
#include <istream>
#include <ostream>
#include <sstream>
#include <vector>
#include <map>
#include <stack>
#include <memory>
#include <limits>
#include <utility>
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
std::ostream &operator<< (std::ostream &os, const statement &x)
{
	x.print(os);
	return os;
}
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
std::ostream &operator<< (std::ostream &os, const assignment &x)
{
	x.print(os);
	return os;
}
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
	assignment step_statement;
	line_num end_for_line;
	FOR(std::unique_ptr<expr::expr> &&_cond,
			assignment &&step,
			const line_num &_end_for_line)
		: condition(std::move(_cond)), step_statement(std::move(step)),
		end_for_line(_end_for_line) {}
	FOR(const std::string &str, const line_num &_end_for_line)
		: condition(expr::parse_expr(std::string(str, str.find(';') + 1))),
		step_statement(std::string(str, 0, str.find(';'))),
		end_for_line(_end_for_line)
	{}
	void print(std::ostream &os) const
	{
		os << "{for (; " << *condition << "; " << step_statement << ")"
			" end_for at line " << end_for_line << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<FOR>
			(condition->deep_copy(), step_statement.copy(), end_for_line);
	}
};
struct END_FOR : statement
{
	line_num for_line;
	END_FOR(const line_num &_for_line) : for_line(_for_line) {}
	void print(std::ostream &os) const
	{
		os << "{end_for of {for} at line " << for_line << '}';
	}
	std::unique_ptr<statement> deep_copy() const
	{
		return std::make_unique<END_FOR>(for_line);
	}
};
using program_type = std::map<line_num, std::unique_ptr<statement>>;
const int additional_exit_line = INT_MAX;
program_type read_program(std::istream &is)
{
	std::map<line_num, std::unique_ptr<statement>> ret;
	line_num line;
	std::stack<std::pair<line_num, std::string>> FOR_stack;
	while (is >> line)
	{
		auto attemp_insert
			= ret.insert(std::make_pair(line, std::unique_ptr<statement>()));
		if (!attemp_insert.second)
			throw "Line number repeated.";
		std::string statement_type;
		is >> statement_type;
		std::unique_ptr<statement> sentence;
		std::string sentence_str;
		getline(is, sentence_str);
		if (statement_type == "REM")
			sentence = std::make_unique<REM>();
		else if (statement_type == "LET")
			sentence = std::make_unique<LET>(sentence_str);
		else if (statement_type == "INPUT")
			sentence = std::make_unique<INPUT>(sentence_str);
		else if (statement_type == "EXIT")
			sentence = std::make_unique<EXIT>(sentence_str);
		else if (statement_type == "GOTO")
			sentence = std::make_unique<GOTO>(sentence_str);
		else if (statement_type == "IF")
			sentence = std::make_unique<IF>(sentence_str);
		else if (statement_type == "FOR")
		{
			FOR_stack.push(std::make_pair(line, sentence_str));
			continue;
		}
		else if (statement_type == "END")
		{
			std::istringstream iss(sentence_str);
			std::string for_expected;
			iss >> for_expected;
			if (for_expected != "FOR")
				throw "Unknown token.";
			char tmp;
			if (iss >> tmp)
				throw "Extra trailing characters.";
			if (FOR_stack.empty())
				throw "Unpaired \"END FOR\".";
			auto &&FOR_info = FOR_stack.top();
			ret[FOR_info.first] = std::make_unique<FOR>(FOR_info.second, line);
			sentence = std::make_unique<END_FOR>(FOR_info.first);
			FOR_stack.pop();
		}
		else
			throw "Unknown token.";
		attemp_insert.first->second = std::move(sentence);
	}
	if (!is.eof())
		throw "Error when reading program. Probably caused by missing line number.";
	if (!FOR_stack.empty())
		throw "Unpaired \"FOR\".";
	if (ret.count(INT_MAX) != 0)
		throw ":-( Line number too big.";
	ret[additional_exit_line] = std::make_unique<EXIT>("0");
	return ret;
}
void print_program(std::ostream &os, const program_type &prog)
{
	for (const auto &[line, sent] : prog)
		os << '#' << line << ":\t" << *sent << std::endl;
}
}
#endif
