#include "../src/link.hpp"
#include <iostream>
int main()
{
	try
	{
		auto &&prog = statement::read_program(std::cin);
		auto &&cfg = basic_block::gen_cfg(prog);
		auto &&obj_code = translate::translate_to_obj_code(cfg);
		auto &&linked_code = link::link(obj_code);
		link::print_linked_code(std::cout, linked_code);
	}
	catch (const char *e)
	{
		std::cerr << e << std::endl;
	}
}
