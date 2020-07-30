#include "translate.hpp"
#include <typeinfo>
namespace translate
{
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
		const auto &var = static_cast<expr::id&>(*e).id_name;
		if (regs.reg_map.count(var) == 0)
			throw "Unknown identifier.";
		int mem_addr = regs.reg_map[var];
		if (target == UNDETERMINED_REG)
		{
			target = mem_addr;
			return {};
		}
		else
		{
			regs.preserve_reg(target);
			if (target < REAL_REG)
				return { inst_mem_2_reg(mem_addr, target) };
			else
				return {
					inst_mem_2_reg(mem_addr, t0),
					inst_reg_2_mem(t0, target)
				};
		}
	}
	int ans = (target >= 0 && target < REAL_REG ? target : t2);
	std::vector<instruction> ret;
	if (type == typeid(expr::imm_num))
	{
		const int &imm = static_cast<expr::imm_num&>(*e).value;
		auto &&[high, low] = split_int32(imm);
		if (high != 0)
			ret = {
				instruction{inst_op::LUI, 0, 0, high, ans},
				instruction{inst_op::ADDI, ans, 0, low, ans}
			};
		else
			ret = {instruction{inst_op::ADDI, zero, 0, low, ans}};
	}
	else if (type == typeid(expr::neg))
	{
		const auto &neg_expr = static_cast<expr::neg&>(*e);
		ret = convert_val_expr(neg_expr.c, ans, regs);
		ret.push_back(instruction{inst_op::SUB, zero, ans, 0, ans});
	}
	else
	{
		const auto &bin_expr = static_cast<expr::bin_op&>(*e);
		int lhs = UNDETERMINED_REG, rhs = UNDETERMINED_REG;
		ret = convert_val_expr(bin_expr.lc, lhs, regs);
		auto &&ret_rhs = convert_val_expr(bin_expr.rc, rhs, regs);
		ret.insert(ret.end(), ret_rhs.begin(), ret_rhs.end());
		if (lhs >= REAL_REG)
		{
			ret.push_back(inst_mem_2_reg(lhs, t0));
			regs.deallocate_reg(lhs);
			lhs = t0;
		}
		if (rhs >= REAL_REG)
		{
			ret.push_back(inst_mem_2_reg(rhs, t1));
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
	{
		if (target == UNDETERMINED_REG)
			target = regs.allocate_reg();
		if (target >= REAL_REG)
			ret.push_back(inst_reg_2_mem(ans, target));
		else
			ret.push_back(inst_reg_2_reg(ans, target));
	}
	regs.preserve_reg(target);
	return ret;
}
std::vector<instruction>
convert_bool_expr(const std::unique_ptr<expr::expr> &e, int &target,
		virtual_reg &regs)
{
	int ans = (target >= 0 && target < REAL_REG ? target : t2);
	const auto &type = typeid(*e);
	if (type != typeid(expr::cmp)
			&& type != typeid(expr::bool_and)
			&& type != typeid(expr::bool_or))
		throw "Error when convert_bool_expr: get val expr where bool expr is expected.";
	const auto &bin_expr = static_cast<expr::bin_op&>(*e);
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
		ret.push_back(inst_mem_2_reg(lhs, t0));
		regs.deallocate_reg(lhs);
		lhs = t0;
	}
	if (rhs >= REAL_REG)
	{
		ret.push_back(inst_mem_2_reg(rhs, t1));
		regs.deallocate_reg(rhs);
		rhs = t1;
	}
	if (type == typeid(expr::cmp))
	{
		const auto &cmp_expr = static_cast<expr::cmp&>(*e);
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
	regs.deallocate_reg(lhs);
	regs.deallocate_reg(rhs);
	if (ans != target)
	{
		if (target == UNDETERMINED_REG)
			target = regs.allocate_reg();
		if (target >= REAL_REG)
			ret.push_back(inst_reg_2_mem(ans, target));
		else
			ret.push_back(inst_reg_2_reg(ans, target));
	}
	regs.preserve_reg(target);
	return ret;
}
obj_code translate_to_obj_code(const basic_block::cfg_type &cfg)
{
	virtual_reg reg_map;
	obj_code ret;
	for (const auto &[line, block] : cfg)
	{
		std::vector<instruction> inst;
		for (const auto &sent : block.commands)
		{
			const auto &type = typeid(*sent);
			std::vector<instruction> sent_inst;
			if (type == typeid(statement::LET))
			{
				const auto &assign =
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
						instruction{inst_op::ECALL, 0, 0, 0, 0},
						inst_reg_2_mem(a0, mem_reg)
					});
				}
			}
			else if (type == typeid(statement::EXIT))
			{
				int exit_val_reg = a1;
				sent_inst = convert_val_expr
					(static_cast<statement::EXIT&>(*sent).val,
						exit_val_reg, reg_map);
				sent_inst.insert(sent_inst.end(), {
					instruction{inst_op::ADDI, zero, 0, CALL_EXIT, a0},
					instruction{inst_op::ECALL, 0, 0, 0, 0}
				});
				reg_map.deallocate_reg(a1);
			}
			else if (type == typeid(statement::IF))
			{
				int branch_flag = a0;
				sent_inst = convert_bool_expr
					(static_cast<statement::IF&>(*sent).condition,
						branch_flag, reg_map);
				reg_map.deallocate_reg(a0);
			}
			else if (type == typeid(statement::FOR))
			{
				int branch_flag = a0;
				sent_inst = convert_bool_expr
					(static_cast<statement::FOR&>(*sent).condition,
						branch_flag, reg_map);
				reg_map.deallocate_reg(a0);
			}
			else if (type == typeid(statement::END_FOR))
			{
				const auto &assign =
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
			inst.insert(inst.end(), sent_inst.begin(), sent_inst.end());
		}
		ret.emplace(line, obj_code_block(std::move(inst),
				block.condition == nullptr
					? nullptr
					: block.condition->deep_copy(),
				block.jump_true, block.jump_false));
	}
	return ret;
}
void print_obj_code_block(std::ostream &os, const obj_code &code)
{
	for (const auto &[line, block] : code)
	{
		os << "{\n  block #" << line << "\n  jump to (";
		if (block.condition == nullptr)
			os << "(true)";
		else
			os << *(block.condition);
		os << " ? " << block.jump_true << " : " << block.jump_false << ")\n";
		for (const auto &inst : block.instructions)
			os << '\t' << inst << '\n';
		os << '}' << std::endl;
	}
}
}
