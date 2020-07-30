#include "to_raw.hpp"
#include <iostream>
int main()
{
	try
	{
		auto &&prog = statement::read_program(std::cin);
		auto &&cfg = basic_block::gen_cfg(prog);
		auto &&obj_code = translate::translate_to_obj_code(cfg);
		auto &&linked_code = link::link(obj_code);
		auto &&raw_prog = to_raw::to_raw_prog(linked_code);
		to_raw::print_raw_prog(std::cout, raw_prog);
	}
	catch (const char *e)
	{
		std::cerr << e << std::endl;
	}
}
