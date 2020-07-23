#ifndef STATEMENT_HPP
#define STATEMENT_HPP
#include "expr.hpp"
#include <istream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <limits>
#include <utility>
namespace statement
{
using line_num = unsigned;
struct statement
{
	virtual ~statement() {}
};
struct REM : statement {};
struct assignment
{
	std::unique_ptr<expr::expr> var;
	std::unique_ptr<expr::expr> val;
	assignment(std::unique_ptr<expr::expr> &&_var,
			std::unique_ptr<expr::expr> &&_val)
		: var(std::move(_var)), val(std::move(_val)) {}
};
struct LET : statement
{
	assignment assign;
	LET(std::unique_ptr<expr::expr> &&var, std::unique_ptr<expr::expr> &&val)
		: assign(std::move(var), std::move(val)) {}
};
struct INPUT : statement
{
	std::vector<std::unique_ptr<expr::expr>> inputs;
	INPUT(std::vector<std::unique_ptr<expr::expr>> &&_inputs)
		: inputs(std::move(_inputs)) {}
};
struct EXIT : statement
{
	std::unique_ptr<expr::expr> val;
	EXIT(std::unique_ptr<expr::expr> &&_val) : val(std::move(_val)) {}
};
struct GOTO : statement
{
	line_num line;
	GOTO(const line_num &_line) : line(_line) {}
};
struct IF : statement
{
	std::unique_ptr<expr::expr> condition;
	line_num line;
};
struct FOR : statement
{
	std::unique_ptr<expr::expr> condition;
	assignment step_statement;
	line_num end_for_line;
};
struct END_FOR : statement
{
	line_num for_line;
};
std::map<line_num, std::unique_ptr<statement>> read_program(std::istream &is)
{
	std::map<line_num, std::unique_ptr<statement>> ret;
	line_num line;
	while (is >> line)
	{
		if (ret.count(line) != 0)
			throw "Line number repeated.";
		std::string statement_type;
		is >> statement_type;
		std::unique_ptr<statement> sentence;
		std::string sentence_str;
		getline(is, sentence_str);
		if (statement_type == "REM")
			sentence = std::make_unique<REM>();
		else if (statement_type == "LET")
		{
			auto equal_sign_pos = sentence_str.find('=');
			auto &&var =
				expr::parse_expr(sentence_str.substr(0, equal_sign_pos));
			auto &&val =
				expr::parse_expr(sentence_str.substr(equal_sign_pos + 1));
			sentence = std::make_unique<LET>(std::move(var), std::move(val));
		}
		else if (statement_type == "INPUT")
		{
			std::vector<std::unique_ptr<expr::expr>> input_list;
			for (size_t pos = 0, nxt_pos = sentence_str.find(',');
					pos != sentence_str.npos;
					pos = nxt_pos, nxt_pos = sentence_str.find(',', pos + 1))
				input_list.push_back(expr::parse_expr(
							sentence_str.substr(pos, nxt_pos - pos)));
			sentence = std::make_unique<INPUT>(std::move(input_list));
		}
		else if (statement_type == "EXIT")
		{
			auto &&exit_expr = expr::parse_expr(sentence_str);
			sentence = std::make_unique<EXIT>(std::move(exit_expr));
		}
		else if (statement_type == "GOTO")
		{
			std::istringstream iss(sentence_str);
			line_num goto_line;
			iss >> goto_line;
			sentence = std::make_unique<GOTO>(goto_line);
		}
		else if (statement_type == "IF")
		{
/*			auto then_pos = sentence_str.find("THEN");
			std::string expr_str(sentence_str, 0, then_pos);
			std::string line_str(sentence_str, then_pos + 4);
			auto &&if_expr = expr::parse_expr(expr_str);
			std::istringstream iss(line_str);
			line_num if_line;
			iss >> if_line;
			std::
*/		}
		else if (statement_type == "FOR")
		{
		}
		else if (statement_type == "END")
		{
		}
		else
			throw "Unknown token.";
	}
}
}
#endif
