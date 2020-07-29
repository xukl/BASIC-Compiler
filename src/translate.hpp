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
			I_case(ADDI)I_case(ORI)I_case(INPUT)I_case(EXIT)I_case(SLTIU)I_case(XORI)
			S_case(SW)
			B_case(BEQ)B_case(BNE)
			U_case(LUI)
			J_case(JAL)
			case inst_op::LW:
				os << "LW x" << rd << ", " << imm << "(x" << rs1 << ")";
		}
	}
};
std::ostream &operator<< (std::ostream &os, const instruction &x)
{
	x.print(os);
	return os;
}
const int zero = 0, sp = 2, t0 = 5, t1 = 6, t2 = 7;
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
	void preserve_var(const std::string &var_name, int elements)
	{
		reg_map[var_name] = memory_reg_end;
		virtual_reg_cnt += elements;
		memory_reg_end += elements;
	}
	int allocate_reg()
	{
		if (aval_reg.empty())
			return ++virtual_reg_cnt;
		auto aval_it = aval_reg.begin();
		int ret = *aval_it;
		aval_reg.erase(aval_it);
		return ret;
	}
	void deallocate_reg(int reg)
	{
		if ((reg >= 10 && reg < REAL_REG) || reg > memory_reg_end)
			aval_reg.insert(reg);
	}
};
const int UNDETERMINED_REG = -1;
std::vector<instruction>
convert_val_expr(const std::unique_ptr<expr::expr> &e, int &target,
		virtual_reg &regs);
std::vector<instruction>
convert_bool_expr(const std::unique_ptr<expr::expr> &e, int &target,
		virtual_reg &regs);
}
#endif
