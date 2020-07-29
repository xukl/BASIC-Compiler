#include "../src/translate.hpp"
#include <iostream>
int main()
{
	try
	{
		auto &&prog = statement::read_program(std::cin);
		auto &&cfg = basic_block::gen_cfg(prog);
		auto &&obj_code = translate::translate_to_obj_code(cfg);
		translate::print_obj_code_block(std::cout, obj_code);
	}
	catch (const char *e)
	{
		std::cerr << e << std::endl;
	}
}
