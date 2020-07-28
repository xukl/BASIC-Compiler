#include "../src/basic_block.hpp"
#include <iostream>
int main()
{
	try
	{
		auto &&prog = statement::read_program(std::cin);
		auto &&cfg = basic_block::gen_cfg(prog);
		basic_block::print_cfg(std::cout, cfg);
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
