#include "../src/statement.hpp"
#include <iostream>
int main()
{
	auto &&prog = statement::read_program(std::cin);
	statement::print_program(std::cout, prog);
}
