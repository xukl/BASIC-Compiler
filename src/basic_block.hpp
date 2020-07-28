#ifndef BASIC_BLOCK_HPP
#define BASIC_BLOCK_HPP
#include "statement.hpp"
namespace basic_block
{
using statement::program_type;
struct basic_block_type
{
	std::vector<std::unique_ptr<statement::statement>> commands;
	std::unique_ptr<expr::expr> condition; // nullptr if unconditional jump
	int jump_true, jump_false; // id for jump target block
	basic_block_type(decltype(commands) &&comms, decltype(condition) &&cond,
			int j_true, int j_false)
		: commands(std::move(comms)), condition(std::move(cond)),
			jump_true(j_true), jump_false(j_false) {}
};
const int BEGIN_IDX = -1, END_IDX = -2;
using cfg_type = std::map<int, basic_block_type>;
cfg_type gen_cfg(const program_type &prog);
void print_cfg(std::ostream &os, const cfg_type &cfg);
}
#endif
