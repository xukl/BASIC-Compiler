#include "statement.hpp"
#include <sstream>
#include <stack>
#include <memory>
#include <utility>
namespace statement
{
std::ostream &operator<< (std::ostream &os, const statement &x)
{
	x.print(os);
	return os;
}
std::ostream &operator<< (std::ostream &os, const assignment &x)
{
	x.print(os);
	return os;
}
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
