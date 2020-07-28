#ifndef BASIC_BLOCK_HPP
#define BASIC_BLOCK_HPP
#include "statement.hpp"
#include <typeinfo>
#include <map>
#include <vector>
#include <set>
#include <cassert>
#include <iostream>
#include <cstring>
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
struct simple_block
{
	int back_line_num;
	int in_edge_cnt, out_edge_cnt;
	int out_edge[2];
};
using num_cfg_type = std::map<int, simple_block>;
inline void add_edge_to(num_cfg_type &m, int u, int v)
{
	m[u].out_edge[m[u].out_edge_cnt++] = v;
	++m[v].in_edge_cnt;
}
num_cfg_type::iterator remove_sent(num_cfg_type &m, int x)
{
	if (x != statement::additional_exit_line)
		std::clog << "Warning: unreachable code at line " << x << std::endl;
	for (int i = 0; i < m[x].out_edge_cnt; ++i)
	{
		auto v = m[x].out_edge[i];
		auto &v_in_cnt = m[v].in_edge_cnt;
		if (--v_in_cnt == 0)
			remove_sent(m, v);
	}
	return m.erase(m.find(x));
}
num_cfg_type gen_num_cfg(const program_type &prog)
{
	num_cfg_type ret;
	ret.emplace(BEGIN_IDX, simple_block());
	ret.emplace(END_IDX, simple_block());
	for (auto it = prog.cbegin(); it != prog.cend(); ++it)
	{
		const auto &[line, sent] = *it;
		const auto &sent_type = typeid(*sent);
		ret[line].back_line_num = line;
		if (sent_type != typeid(statement::EXIT)
				&& sent_type != typeid(statement::GOTO)
				&& sent_type != typeid(statement::END_FOR))
		{
			auto nxt = it;
			++nxt;
			if (nxt == prog.cend())
				add_edge_to(ret, line, END_IDX);
			else
				add_edge_to(ret, line, nxt->first);
		}
		if (sent_type == typeid(statement::EXIT))
			add_edge_to(ret, line, END_IDX);
		else if (sent_type == typeid(statement::GOTO))
			add_edge_to(ret, line,
					static_cast<statement::GOTO&>(*sent).line);
		else if (sent_type == typeid(statement::IF))
			add_edge_to(ret, line,
					static_cast<statement::IF&>(*sent).line);
		else if (sent_type == typeid(statement::FOR))
		{
			auto end_for_line =
				static_cast<statement::FOR&>(*sent).end_for_line;
			auto jump_pos = prog.upper_bound(end_for_line);
			if (jump_pos == prog.cend())
				add_edge_to(ret, line, END_IDX);
			else
				add_edge_to(ret, line, jump_pos->first);
		}
		else if (sent_type == typeid(statement::END_FOR))
			add_edge_to(ret, line,
					static_cast<statement::END_FOR&>(*sent).for_line);
	}
	add_edge_to(ret, BEGIN_IDX, prog.cbegin()->first);
	for (auto it = ret.lower_bound(0); it != ret.end(); )
		if (it->second.in_edge_cnt == 0)
			it = remove_sent(ret, it->first);
		else
			++it;
	for (auto it = ret.lower_bound(0); it != ret.end(); ++it)
	{
		auto nxt = it;
		++nxt;
		if (nxt == ret.end())
			break;
		while (it->second.out_edge_cnt == 1 &&
				nxt->second.in_edge_cnt == 1 &&
				it->second.out_edge[0] == nxt->first)
		{
			it->second.back_line_num = nxt->first;
			memcpy(it->second.out_edge, nxt->second.out_edge,
					sizeof(it->second.out_edge));
			nxt = ret.erase(nxt);
			if (nxt == ret.end())
				break;
		}
	}
	return ret;
}
using cfg_type = std::map<int, basic_block_type>;
cfg_type gen_cfg(const program_type &prog)
{
	const auto &simple_cfg = gen_num_cfg(prog);
	cfg_type ret;
	for (auto &&num_cfg_node = simple_cfg.lower_bound(0);
			num_cfg_node != simple_cfg.cend();
			++num_cfg_node)
	{
		int begin_line = num_cfg_node->first,
			back_line = num_cfg_node->second.back_line_num;
		std::vector<std::unique_ptr<statement::statement>> block_statement;
		for (auto &&it_prog = prog.find(begin_line);
				it_prog != prog.cend() && it_prog->first <= back_line;
				++it_prog)
			block_statement.push_back(it_prog->second->deep_copy());
		const auto &last_sent = block_statement.back();
		const auto &sent_type = typeid(*last_sent);
		std::unique_ptr<expr::expr> condition = nullptr;
		int jump_true = num_cfg_node->first, jump_false = num_cfg_node->first;
		if (sent_type != typeid(statement::IF) &&
				sent_type != typeid(statement::FOR))
			jump_true = num_cfg_node->second.out_edge[0];
		else if (sent_type == typeid(statement::IF))
		{
			jump_false = num_cfg_node->second.out_edge[0];
			jump_true = num_cfg_node->second.out_edge[1];
			condition =
				static_cast<statement::IF&>(*last_sent).condition->deep_copy();
		}
		else if (sent_type == typeid(statement::FOR))
		{
			jump_true = num_cfg_node->second.out_edge[0];
			jump_false = num_cfg_node->second.out_edge[1];
			condition =
				static_cast<statement::FOR&>(*last_sent).condition->deep_copy();
		}
		ret.emplace(num_cfg_node->first, basic_block_type
				(std::move(block_statement), std::move(condition),
				 jump_true, jump_false));
	}
	return ret;
}
void print_cfg(std::ostream &os, const cfg_type &cfg)
{
	for (const auto &[line, block] : cfg)
	{
		os << "{\n  block from line " << line << ",\n  condition: ";
		if (block.condition == nullptr)
			os << "(true)\n";
		else
			os << *(block.condition) << '\n';
		os << "  jump_true: " << block.jump_true
			<< "\tjump_false: " << block.jump_false << '\n';
		for (const auto &sentence : block.commands)
			os << '\t' << *sentence << '\n';
		os << "}\n";
	}
	os << std::endl;
}
}
#endif
