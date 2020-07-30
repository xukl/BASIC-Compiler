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
using namespace inst;
struct virtual_reg
{
	int virtual_reg_cnt, memory_reg_end;
	std::set<int> aval_reg;
	std::map<std::string, int> reg_map;
	virtual_reg() : virtual_reg_cnt(REAL_REG), memory_reg_end(REAL_REG)
	{
		for (int i = 10; i < REAL_REG; ++i)
			aval_reg.insert(i);
	}
	int preserve_var(const std::string &var_name, int elements)
	{
		if (reg_map.count(var_name) != 0)
			return reg_map[var_name];
		int ret = memory_reg_end;
		reg_map[var_name] = memory_reg_end;
		virtual_reg_cnt += elements;
		memory_reg_end += elements;
		return ret;
	}
	void preserve_reg(int reg)
	{
		aval_reg.erase(reg);
	}
	int allocate_reg()
	{
		if (aval_reg.empty())
			return ++virtual_reg_cnt;
		int ret = *aval_reg.begin();
		preserve_reg(ret);
		return ret;
	}
	void deallocate_reg(int reg)
	{
		if ((reg >= 10 && reg < REAL_REG) || reg > memory_reg_end)
			aval_reg.insert(reg);
	}
};
struct obj_code_block
{
	std::vector<instruction> instructions;
	std::unique_ptr<expr::expr> condition;
	int jump_true, jump_false;
	obj_code_block(std::vector<instruction> &&_insts,
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
