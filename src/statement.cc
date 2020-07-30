#include "statement.hpp"
#include <sstream>
#include <stack>
#include <memory>
#include <utility>
#include <typeinfo>
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
			const auto &str = FOR_info.second;
			ret[FOR_info.first] = std::make_unique<FOR>(str, line);
			sentence = std::make_unique<END_FOR>(FOR_info.first,
					assignment(std::string(str, 0, str.find(';'))));
			FOR_stack.pop();
		}
		else
			throw "Unknown token.";
		attemp_insert.first->second = std::move(sentence);
	}
	for (auto &[line, sent] : ret)
	{
		const auto &sent_type = typeid(*sent);
		if (sent_type == typeid(IF))
		{
			line_num &target_line = static_cast<IF&>(*sent).line;
			if (typeid(*ret[target_line]) == typeid(FOR))
			{
				line_num end_for_line =
					static_cast<FOR&>(*ret[target_line]).end_for_line;
				if (line >= target_line && line <= end_for_line)
					target_line = end_for_line;
			}
		}
		if (sent_type == typeid(GOTO))
		{
			line_num &target_line = static_cast<GOTO&>(*sent).line;
			if (typeid(*ret[target_line]) == typeid(FOR))
			{
				line_num end_for_line =
					static_cast<FOR&>(*ret[target_line]).end_for_line;
				if (line >= target_line && line <= end_for_line)
					target_line = end_for_line;
			}
		}
	}
	if (!is.eof())
		throw "Error when reading program. Probably caused by missing line number.";
	if (!FOR_stack.empty())
		throw "Unpaired \"FOR\".";
	if (ret.count(INT_MAX) != 0)
		throw ":-( Line number too big.";
	ret[additional_exit_line]
		= std::make_unique<EXIT>(std::make_unique<expr::imm_num>(0));
	return ret;
}
void print_program(std::ostream &os, const program_type &prog)
{
	for (const auto &[line, sent] : prog)
		os << '#' << line << ":\t" << *sent << std::endl;
}
}
