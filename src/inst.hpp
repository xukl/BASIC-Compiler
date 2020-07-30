#ifndef INST_HPP
#define INST_HPP
#include <utility>
namespace inst
{
enum class inst_op { ADD, SUB, MUL, DIV, ADDI, LUI, ORI, LW, SW, JALR, INPUT, EXIT, AND, OR, SLTIU, SLT, BEQ, BNE, XORI, AUIPC };
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
			I_case(ADDI)I_case(ORI)I_case(SLTIU)I_case(XORI)I_case(JALR)
			S_case(SW)
			B_case(BEQ)B_case(BNE)
			U_case(LUI)U_case(AUIPC)
#undef R_case
#undef I_case
#undef S_case
#undef B_case
#undef U_case
#undef J_case
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
inline std::ostream &operator<< (std::ostream &os, const instruction &x)
{
	x.print(os);
	return os;
}
const int zero = 0, sp = 2, t0 = 5, t1 = 6, t2 = 7, a0 = 10, a1 = 11;
const static int REAL_REG = 32;
inline constexpr instruction inst_mem_2_reg(int mem, int reg)
{
	return instruction{inst_op::LW, sp, 0, -4 * (mem - REAL_REG + 1), reg};
}
inline constexpr instruction inst_reg_2_reg(int rs, int rd)
{
	return instruction{inst_op::ADDI, rs, 0, 0, rd};
}
inline constexpr instruction inst_reg_2_mem(int reg, int mem)
{
	return instruction{inst_op::SW, sp, reg, -4 * (mem - REAL_REG + 1), 0};
}
inline constexpr instruction inst_NOP
	= instruction{inst_op::ADDI, zero, 0, 0, zero};
inline std::pair<int, int> split_int32(int val) // return {high, low}
{
	int low = val << 20 >> 20, high = val - low;
	return std::make_pair(high, low);
}
}
#endif
