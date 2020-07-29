#include "translate.hpp"
#include <typeinfo>
namespace translate
{
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
		const std::string &var = static_cast<expr::id&>(*e).id_name;
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
		const int &imm = static_cast<expr::imm_num&>(*e).value;
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
		const expr::neg &neg_expr = static_cast<expr::neg&>(*e);
		ret = convert_val_expr(neg_expr.c, ans, regs);
		ret.push_back(instruction{inst_op::SUB, zero, ans, 0, ans});
	}
	else
	{
		const expr::bin_op &bin_expr = static_cast<expr::bin_op&>(*e);
		int lhs = UNDETERMINED_REG, rhs = UNDETERMINED_REG;
		ret = convert_val_expr(bin_expr.lc, lhs, regs);
		auto &&ret_rhs = convert_val_expr(bin_expr.rc, rhs, regs);
		ret.insert(ret.end(), ret_rhs.begin(), ret_rhs.end());
		if (lhs >= REAL_REG)
		{
			ret.push_back(instruction{inst_op::LW, sp, 0,
					-4 * (lhs - REAL_REG + 1), t0});
			regs.deallocate_reg(lhs);
			lhs = t0;
		}
		if (rhs >= REAL_REG)
		{
			ret.push_back(instruction{inst_op::LW, sp, 0,
					-4 * (rhs - REAL_REG + 1), t1});
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
		ret.push_back(instruction{inst_op::SW, sp, ans,
				-4 * (target - REAL_REG + 1), 0});
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
	const expr::bin_op &bin_expr = static_cast<expr::bin_op&>(*e);
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
		ret.push_back(instruction{inst_op::LW, sp, 0,
				-4 * (lhs - REAL_REG + 1), t0});
		regs.deallocate_reg(lhs);
		lhs = t0;
	}
	if (rhs >= REAL_REG)
	{
		ret.push_back(instruction{inst_op::LW, sp, 0,
				-4 * (rhs - REAL_REG + 1), t1});
		regs.deallocate_reg(rhs);
		rhs = t1;
	}
	if (type == typeid(expr::cmp))
	{
		const expr::cmp &cmp_expr = static_cast<expr::cmp&>(*e);
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
		ret.push_back(instruction{inst_op::SW, sp, ans,
				-4 * (target - REAL_REG + 1), 0});
	regs.deallocate_reg(lhs);
	regs.deallocate_reg(rhs);
	return ret;
}
obj_code trarnslate_to_obj_code(const basic_block::cfg_type &cfg)
{
	virtual_reg reg_map;
	obj_code ret;
	for (const auto &[line, block] : cfg)
	{
		std::vector<instruction> inst;
		for (const auto &sent : block.commands)
		{
			const auto &type = typeid(sent);
			std::vector<instruction> sent_inst;
			if (type == typeid(statement::LET))
			{
				const statement::assignment &assign =
					static_cast<statement::LET&>(*sent).assign;
				if (typeid(*(assign.val)) == typeid(expr::subscript))
					throw "subscript is not supported yet.";
				if (typeid(*(assign.var)) != typeid(expr::id))
					throw "lvalue expected in {LET} command.";
				int mem_reg = reg_map.preserve_var
					(static_cast<expr::id&>(*(assign.var)).id_name, 1);
				sent_inst = convert_val_expr(assign.val, mem_reg, reg_map);
			}
			else if (type == typeid(statement::INPUT))
			{
				const auto &inputs =
					static_cast<statement::INPUT&>(*sent).inputs;
				for (const auto &var : inputs)
				{
					const auto &type_var = typeid(*var);
					if (type_var == typeid(expr::subscript))
						throw "subscript is not supported yet.";
					if (type_var != typeid(expr::id))
						throw "lvalue expected in {INPUT} command.";
					const auto &name = static_cast<expr::id&>(*var).id_name;
					auto mem_reg = reg_map.preserve_var(name, 1);
					sent_inst.insert(sent_inst.end(), {
						instruction{inst_op::ADDI, zero, 0, CALL_READ, a0},
						instruction{inst_op::INPUT, 0, 0, 0, 0},
						instruction{inst_op::SW, sp, a0,
							-4 * (mem_reg - REAL_REG + 1), 0}
					});
				}
			}
			else if (type == typeid(statement::EXIT))
			{
				int exit_val_reg = a1;
				reg_map.preserve_reg(a1);
				sent_inst = convert_val_expr
					(static_cast<statement::EXIT&>(*sent).val,
						exit_val_reg, reg_map);
				sent_inst.insert(sent_inst.end(), {
					instruction{inst_op::ADDI, zero, 0, CALL_EXIT, a0},
					instruction{inst_op::EXIT, 0, 0, 0, 0}
				});
				reg_map.deallocate_reg(a1);
			}
			else if (type == typeid(statement::IF))
			{
				int branch_flag = a0;
				reg_map.preserve_reg(a0);
				sent_inst = convert_bool_expr
					(static_cast<statement::IF&>(*sent).condition,
						branch_flag, reg_map);
				reg_map.deallocate_reg(a0);
			}
			else if (type == typeid(statement::FOR))
			{
				int branch_flag = a0;
				reg_map.preserve_reg(a0);
				sent_inst = convert_bool_expr
					(static_cast<statement::FOR&>(*sent).condition,
						branch_flag, reg_map);
				reg_map.deallocate_reg(a0);
			}
			else if (type == typeid(statement::END_FOR))
			{
				const statement::assignment &assign =
					static_cast<statement::END_FOR&>(*sent).step_statement;
				if (typeid(*(assign.val)) == typeid(expr::subscript))
					throw "subscript is not supported yet.";
				if (typeid(*(assign.var)) != typeid(expr::id))
					throw "lvalue expected in {LET} command.";
				int mem_reg = reg_map.preserve_var
					(static_cast<expr::id&>(*(assign.var)).id_name, 1);
				sent_inst = convert_val_expr(assign.val, mem_reg, reg_map);
			}
			else
			{
			}
		}
		ret.emplace(line,
				obj_code_block(std::move(inst), block.condition->deep_copy(),
					block.jump_true, block.jump_false));
	}
	return ret;
}
}
