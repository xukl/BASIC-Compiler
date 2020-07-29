#include "basic_block.hpp"
#include <map>
#include <typeinfo>
namespace translate
{
enum class inst_op { ADD, SUB, MUL, DIV, ADDI, LUI, ORI, LW, SW, JAL, INPUT, EXIT };
struct instruction
{
	inst_op op;
	int rs1, rs2, imm, rd;
};
struct virtual_reg
{
	int max_reg_id;
	std::set<int> aval_reg;
	std::map<expr::id, int> reg_map;
	virtual_reg() : max_reg_id(31)
	{
		for (int i = 10; i <= 31; ++i)
			aval_reg.insert(i);
	}
};
void deallocate_reg(virtual_reg &regs, int reg)
{
	if (reg_map.count(reg) == 0) // not an id
		regs.aval_reg.insert(reg);
}
int allocate_reg(virtual_reg &regs)
{
	if (regs.aval_reg.empty())
		return ++regs.max_reg_id;
	auto aval_it = *regs.aval_reg.begin();
	int ret = *aval_it;
	regs,aval_reg.erase(aval);
	return ret;
}
const int NON_DETERMINED_REG = -1;
std::vector<instruction>
convert_val_expr(const std::unique_ptr<expr::expr> &e, int &target, virtual_reg &regs)
{
	if (typeid(*e) == typeid(expr::id))
	{
		const expr::id &var = static_cast<const expr::id&>(*e).id_name;
		assert(regs.reg_map.count(var) != 0);
		target = regs.reg_map[var];
		return {};
	}
	if (target == NON_DETERMINED_REG)
		target = allocate_reg(regs);
	if (typeid(*e) == typeid(expr::imm_num))
	{
		const int &imm = static_cast<const expe::imm_num&>(*e).value;
		int high = imm & 0xfffff000, low = imm & 0xfff;
		if (imm == (imm << 20 >> 20))
			return { instruction{inst_op::ADDI, 0, 0, imm, target} };
		return {
			instruction{inst_op::LUI, 0, 0, high, rd},
			instruction{inst_op::ORI, rd, 0, low, rd}
		};
	}
#define convert_bin_op(expr_type, inst_type)\
	if (typeid(*e) == typeid(expr_type))\
	{\
		const expr_type &type_expr = static_cast<const expr_type&>(*e);\
		int lhs = NON_DETERMINED_REG;\
		auto &&ret_lhs = convert_val_expr(type_expr.lc, lhs, regs);\
		int rhs = NON_DETERMINED_REG;\
		auto &&ret_rhs = convert_val_expr(type_expr.rc, rhs, regs);\
		ret_lhs.insert(ret_lhs.end(), ret_rhs.begin(), ret_rhs.end());\
		ret_lhs.push_back({instruction(inst_type, lhs, rhs, 0, target)});\
		deallocate_reg(regs, lhs);\
		deallocate_reg(regs, rhs);\
		return std::move(ret_lhs);\
	}
	convert_bin_op(expr::add, inst_op::ADD)
	convert_bin_op(expr::sub, inst_op::SUB)
	convert_bin_op(expr::mul, inst_op::MUL)
	convert_bin_op(expr::div, inst_op::DIV)
#undef convert_bin_op
	if (typeid(*e) == typeid(expr::neg))
	{
		const expr::neg &neg_expr = static_cast<const expr::neg&>(*e);
		auto &&ret = convert_val_expr(neg_expr.c, target, regs);
		ret.push_back(instruction{inst_op::SUB, 0, target, 0, target});
		return std::move(ret);
	}
	if (typeid(*e) == typeid(expr::subscript))
		throw "subscript is not supported yet.";
	throw "Error when convert_val_expr: get bool expr where val expr is expected.";
}
}
