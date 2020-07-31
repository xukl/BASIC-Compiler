#ifndef TRANSLATE_HPP
#define TRANSLATE_HPP
#include "basic_block.hpp"
#include "inst.hpp"
#include <ostream>
#include <map>
#include <set>
#include <vector>
namespace translate
{
struct obj_code_block
{
	std::vector<inst::instruction> instructions;
	std::unique_ptr<expr::expr> condition;
	int jump_true, jump_false;
	obj_code_block(std::vector<inst::instruction> &&_insts,
			std::unique_ptr<expr::expr> &&cond,
			int j_true, int j_false)
		: instructions(std::move(_insts)), condition(std::move(cond)),
		jump_true(j_true), jump_false(j_false) {}
};
using obj_code = std::map<int, obj_code_block>;
obj_code translate_to_obj_code(const basic_block::cfg_type &cfg);
void print_obj_code_block(std::ostream &os, const obj_code &code);
}
#endif
