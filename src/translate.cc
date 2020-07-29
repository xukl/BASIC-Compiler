#include "basic_block.hpp"
#include <map>
#include <set>
#include <typeinfo>
namespace translate
{
enum class inst_op { ADD, SUB, MUL, DIV, ADDI, LUI, ORI, LW, SW, JAL, INPUT, EXIT, AND, OR, SLTIU, SLT, BEQ, BNE, XORI };
struct instruction
{
	inst_op op;
	int rs1, rs2, imm, rd;
};
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
		virtual_reg &regs)
{
	const auto &type = typeid(*e);
	if (type == typeid(expr::cmp) ||
			type == typeid(expr::bool_and) ||
			type == typeid(expr::bool_or))
		throw "Error when convert_val_expr: get bool expr where val expr is expected.";
	else if (type == typeid(expr::subscript))
		throw "subscript is not supported yet.";
	if (type == typeid(expr::id))
	{
		const std::string &var = static_cast<const expr::id&>(*e).id_name;
		if (regs.reg_map.count(var) == 0)
			throw "Unknown identifier.";
		target = regs.reg_map[var];
		return {};
	}
	if (target == UNDETERMINED_REG)
		target = regs.allocate_reg();
	int ans = (target < REAL_REG ? target : t2);
	std::vector<instruction> ret;
	if (type == typeid(expr::imm_num))
	{
		const int &imm = static_cast<const expr::imm_num&>(*e).value;
		int high = imm & 0xfffff000, low = imm & 0xfff;
		if (imm == (imm << 20 >> 20))
			ret = { instruction{inst_op::ADDI, zero, 0, imm, ans} };
		else
			ret = {
				instruction{inst_op::LUI, 0, 0, high, ans},
				instruction{inst_op::ORI, ans, 0, low, ans}
			};
	}
	else if (type == typeid(expr::neg))
	{
		const expr::neg &neg_expr = static_cast<const expr::neg&>(*e);
		ret = convert_val_expr(neg_expr.c, ans, regs);
		ret.push_back(instruction{inst_op::SUB, zero, ans, 0, ans});
	}
	else
	{
		const expr::bin_op &bin_expr = static_cast<const expr::bin_op&>(*e);
		int lhs = UNDETERMINED_REG, rhs = UNDETERMINED_REG;
		ret = convert_val_expr(bin_expr.lc, lhs, regs);
		auto &&ret_rhs = convert_val_expr(bin_expr.rc, rhs, regs);
		ret.insert(ret.end(), ret_rhs.begin(), ret_rhs.end());
		if (lhs >= REAL_REG)
		{
			ret.push_back(instruction{inst_op::LW, sp, 0, -lhs, t0});
			regs.deallocate_reg(lhs);
			lhs = t0;
		}
		if (rhs >= REAL_REG)
		{
			ret.push_back(instruction{inst_op::LW, sp, 0, -rhs, t1});
			regs.deallocate_reg(rhs);
			rhs = t1;
		}
		if (type == typeid(expr::add))
			ret.push_back(instruction{inst_op::ADD, lhs, rhs, 0, ans});
		else if (type == typeid(expr::sub))
			ret.push_back(instruction{inst_op::SUB, lhs, rhs, 0, ans});
		else if (type == typeid(expr::mul))
			ret.push_back(instruction{inst_op::MUL, lhs, rhs, 0, ans});
		else if (type == typeid(expr::div))
			ret.push_back(instruction{inst_op::DIV, lhs, rhs, 0, ans});
		regs.deallocate_reg(lhs);
		regs.deallocate_reg(rhs);
	}
	if (ans != target)
		ret.push_back(instruction{inst_op::SW, sp, ans, -target, 0});
	return ret;
}
std::vector<instruction>
convert_bool_expr(const std::unique_ptr<expr::expr> &e, int &target,
		virtual_reg &regs)
{
	if (target == UNDETERMINED_REG)
		target = regs.allocate_reg();
	int ans = (target < REAL_REG ? target : t2);
	const auto &type = typeid(*e);
	if (type != typeid(expr::cmp)
			&& type != typeid(expr::bool_and)
			&& type != typeid(expr::bool_or))
		throw "Error when convert_bool_expr: get val expr where bool expr is expected.";
	const expr::bin_op &bin_expr = static_cast<const expr::bin_op&>(*e);
	std::vector<instruction> ret, ret_rhs;
	int lhs = UNDETERMINED_REG, rhs = UNDETERMINED_REG;
	if (type == typeid(expr::cmp))
	{
		ret = convert_val_expr(bin_expr.lc, lhs, regs);
		ret_rhs = convert_val_expr(bin_expr.rc, rhs, regs);
	}
	else
	{
		ret = convert_bool_expr(bin_expr.lc, lhs, regs);
		ret_rhs = convert_bool_expr(bin_expr.rc, rhs, regs);
	}
	ret.insert(ret.end(), ret_rhs.begin(), ret_rhs.end());
	if (lhs >= REAL_REG)
	{
		ret.push_back(instruction{inst_op::LW, sp, 0, -lhs, t0});
		regs.deallocate_reg(lhs);
		lhs = t0;
	}
	if (rhs >= REAL_REG)
	{
		ret.push_back(instruction{inst_op::LW, sp, 0, -rhs, t1});
		regs.deallocate_reg(rhs);
		rhs = t1;
	}
	if (type == typeid(expr::cmp))
	{
		const expr::cmp &cmp_expr = static_cast<const expr::cmp&>(*e);
		switch (cmp_expr.op)
		{
			case expr::cmp::LT:
			case expr::cmp::GE:
				ret.push_back(instruction{inst_op::SLT, lhs, rhs, 0, ans});
				break;
			case expr::cmp::GT:
			case expr::cmp::LE:
				ret.push_back(instruction{inst_op::SLT, rhs, lhs, 0, ans});
				break;
			default:
				ret.push_back(instruction{inst_op::SUB, lhs, rhs, 0, ans});
				ret.push_back(instruction{inst_op::SLTIU, ans, 0, 1, ans});
		}
		switch (cmp_expr.op)
		{
			case expr::cmp::GE:
			case expr::cmp::LE:
			case expr::cmp::NE:
				ret.push_back(instruction{inst_op::XORI, ans, 0, 1, ans});
				break;
			default:
				;
		}
	}
	else if (type == typeid(expr::bool_and))
		ret.push_back(instruction{inst_op::AND, lhs, rhs, 0, ans});
	else
		ret.push_back(instruction{inst_op::OR, lhs, rhs, 0, ans});
	if (ans != target)
		ret.push_back(instruction{inst_op::SW, sp, ans, -target, 0});
	regs.deallocate_reg(lhs);
	regs.deallocate_reg(rhs);
	return ret;
}
}
