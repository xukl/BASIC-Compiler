#include "../src/statement.hpp"
#include <iostream>
int main()
{
	try
	{
		auto &&prog = statement::read_program(std::cin);
		statement::print_program(std::cout, prog);
		statement::program_type copy_prog;
		for (const auto &[line, sent] : prog)
			copy_prog.emplace(line, sent->deep_copy());
		statement::print_program(std::cout, copy_prog);
	}
	catch (const char *e)
	{
		std::cerr << e << std::endl;
	}
	catch (...)
	{
		throw;
	}
}
