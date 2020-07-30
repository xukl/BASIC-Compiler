#include "to_raw.hpp"
namespace to_raw
{
using namespace inst;
raw_prog to_raw_prog(const link::linked_prog &code)
{
	raw_prog ret;
	for (const auto &inst : code)
	{
		uint32_t raw_inst;
		switch (inst.op)
		{
#define R_type_code(op_type, funct7, funct3, opcode) \
			case inst_op::op_type:\
				raw_inst =\
					(funct7 << 25) |\
					(inst.rs2 << 20) |\
					(inst.rs1 << 15) |\
					(funct3 << 12) |\
					(inst.rd << 7) |\
					int(inst_opcode::opcode);\
				break;
			R_type_code(ADD, 0b0000000, 0b000, OP)
			R_type_code(SUB, 0b0100000, 0b000, OP)
			R_type_code(SLT, 0b0000000, 0b010, OP)
			R_type_code(OR , 0b0000000, 0b110, OP)
			R_type_code(AND, 0b0000000, 0b111, OP)
			R_type_code(MUL, 0b0000001, 0b000, OP)
			R_type_code(DIV, 0b0000001, 0b100, OP)
#undef R_type_code
#define I_type_code(op_type, funct3, opcode) \
			case inst_op::op_type:\
				raw_inst =\
					(inst.imm << 20) |\
					(inst.rs1 << 15) |\
					(funct3 << 12) |\
					(inst.rd << 7) |\
					int(inst_opcode::opcode);\
				break;
			I_type_code(ADDI , 0b000, OP_IMM)
			I_type_code(SLTIU, 0b011, OP_IMM)
			I_type_code(XORI , 0b100, OP_IMM)
			I_type_code(JALR , 0b000, JALR)
			I_type_code(LW   , 0b010, LOAD)
			I_type_code(ECALL, 0b000, SYSTEM)
#undef I_type_code
#define S_type_code(op_type, funct3, opcode) \
			case inst_op::op_type:\
				raw_inst =\
					(inst.imm >> 5 << 25) |\
					(inst.rs2 << 20) |\
					(inst.rs1 << 15) |\
					(funct3 << 12) |\
					((inst.imm & ((1 << 5) - 1)) << 7) |\
					int(inst_opcode::opcode);\
				break;
			S_type_code(SW, 0b010, STORE)
#undef S_type_code
#define B_type_code(op_type, funct3, opcode) \
			case inst_op::op_type:\
				raw_inst =\
					(inst.imm >> 12 << 31) |\
					(((inst.imm >> 5) & ((1 << 6) - 1)) << 25) |\
					(inst.rs2 << 20) |\
					(inst.rs1 << 15) |\
					(funct3 << 12) |\
					(((inst.imm >> 1) & ((1 << 4) - 1)) << 8) |\
					(((inst.imm >> 11) & ((1 << 1) - 1)) << 7) |\
					int(inst_opcode::opcode);\
				break;
			B_type_code(BEQ, 0b000, BRANCH)
#undef B_type_code
#define U_type_code(op_type, opcode) \
			case inst_op::op_type:\
				raw_inst = inst.imm | (inst.rd << 7) | int(inst_opcode::opcode);\
				break;
			U_type_code(LUI, LUI)
			U_type_code(AUIPC, AUIPC)
#undef U_type_code
#define J_type_code(op_type, opcode) \
			case inst_op::op_type:\
				raw_inst =\
					(inst.imm >> 20 << 31) |\
					(((inst.imm >> 1) & ((1 << 10) - 1)) << 24) |\
					(((inst.imm >> 11) & ((1 << 1) - 1)) << 20) |\
					(((inst.imm >> 12) & ((1 << 8) - 1)) << 12) |\
					(inst.rd << 7) |\
					int(inst_opcode::opcode);\
				break;
#undef J_type_code
		}
		ret.push_back(raw_inst & 0xff);
		ret.push_back((raw_inst >> 8) & 0xff);
		ret.push_back((raw_inst >> 16) & 0xff);
		ret.push_back(raw_inst >> 24);
	}
	return ret;
}
void print_raw_prog(std::ostream &os, const raw_prog &prog)
{
	const auto &os_flag = os.flags();
	os << "@00000000\n";
	os << std::hex;
	for (size_t i = 0; i < prog.size(); ++i)
	{
		os.width(2);
		os.fill('0');
		os << unsigned(prog[i]);
		if ((i + 1) % 8 == 0)
			os << '\n';
		else
			os << ' ';
	}
	os.flags(os_flag);
}
}
