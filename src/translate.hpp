#ifndef TRANSLATE_HPP
#define TRANSLATE_HPP
#include "basic_block.hpp"
#include <ostream>
#include <map>
#include <set>
#include <vector>
namespace translate
{
enum class inst_op { ADD, SUB, MUL, DIV, ADDI, LUI, ORI, LW, SW, JAL, INPUT, EXIT, AND, OR, SLTIU, SLT, BEQ, BNE, XORI };
const int CALL_EXIT = 0, CALL_READ = 1, CALL_PRINT = 2;
struct instruction
{
	inst_op op;
	int rs1, rs2, imm, rd;
	void print(std::ostream &os) const
	{
		switch (op)
		{
#define R_case(op_type)\
			case inst_op::op_type:\
				os << #op_type << " x" << rd << ", x" << rs1 << ", x" << rs2;\
				break;
#define I_case(op_type)\
			case inst_op::op_type:\
				os << #op_type << " x" << rd << ", x" << rs1 << ", " << imm;\
				break;
#define S_case(op_type)\
			case inst_op::op_type:\
				os << #op_type << ' ' << imm << "(x" << rs1 << "), x" << rs2;\
				break;
#define B_case(op_type)\
			case inst_op::op_type:\
				os << #op_type << " x" << rs1 << ", x" << rs2 << ", " << imm;\
				break;
#define U_case(op_type)\
			case inst_op::op_type:\
				os << #op_type << " x" << rd << ", " << imm;\
				break;
#define J_case(op_type)\
			case inst_op::op_type:\
				os << #op_type << " x" << rd << ", " << imm;\
				break;
			R_case(ADD)R_case(SUB)R_case(MUL)R_case(DIV)R_case(AND)R_case(OR)R_case(SLT)
			I_case(ADDI)I_case(ORI)I_case(SLTIU)I_case(XORI)
			S_case(SW)
			B_case(BEQ)B_case(BNE)
			U_case(LUI)
			J_case(JAL)
			case inst_op::INPUT:
				os << "INPUT";
				break;
			case inst_op::EXIT:
				os << "EXIT";
				break;
			case inst_op::LW:
				os << "LW x" << rd << ", " << imm << "(x" << rs1 << ")";
		}
	}
};
std::ostream &operator<< (std::ostream &os, const instruction &x);
const int zero = 0, sp = 2, t0 = 5, t1 = 6, t2 = 7, a0 = 10, a1 = 11;
const static int REAL_REG = 32;
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
