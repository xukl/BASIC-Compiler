#include "../src/expr.hpp"
#include <iostream>
#include <string>

int main()
{
	std::string s;
	while (std::getline(std::cin, s))
	{
		try
		{
			std::cout << *expr::parse_expr(s) << std::endl;
		}
		catch (const char *s)
		{
			std::cerr << "ERROR: " << s << std::endl;
		}
	}
}
