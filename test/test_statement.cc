#include "../src/statement.hpp"
#include <iostream>
int main()
{
	try
	{
		auto &&prog = statement::read_program(std::cin);
		statement::print_program(std::cout, prog);
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
