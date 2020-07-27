#ifndef BASIC_BLOCK_HPP
#define BASIC_BLOCK_HPP
#include "statement.hpp"
#include <typeinfo>
#include <map>
#include <vector>
#include <set>
#include <cassert>
#include <iostream>
namespace basic_block
{
using statement::program_type;
struct basic_block_type
{
	std::vector<std::unique_ptr<statement::statement>> commands;
	std::unique_ptr<expr::expr> branch_cond; // nullptr if unconditional jump
	basic_block_type *jump_true, *jump_false;
	basic_block_type(std::unique_ptr<statement::statement> &&sent = nullptr)
		: commands{std::move(sent)}, jump_true(nullptr), jump_false(nullptr) {}
};
const int BEGIN_IDX = -1, END_IDX = -2;
struct simple_block
{
	int back_line_num;
	std::set<int> in_edge, out_edge;
};
inline void add_edge_to(std::map<int, simple_block> &m, int u, int v)
{
	m[u].out_edge.insert(v);
	m[v].in_edge.insert(u);
}
void remove_sent(std::map<int, simple_block> &m, int x)
{
	std::clog << "Warning: unreachable code at line " << x << std::endl;
	for (auto v : m[x].out_edge)
	{
		auto &v_in = m[v].in_edge;
		v_in.erase(x);
		if (v_in.size() == 0)
			remove_sent(m, v);
	}
	m.erase(x);
}
using cfg_type = std::map<int, basic_block_type>;
cfg_type gen_cfg(const program_type &prog)
{
	std::map<int, simple_block> simple_cfg;
	simple_cfg.emplace(BEGIN_IDX, simple_block());
	simple_cfg.emplace(END_IDX, simple_block());
	for (auto it = prog.cbegin(); it != prog.cend(); ++it)
	{
		const auto &[line, sent] = *it;
		const auto &tmp = typeid(*sent);
		simple_cfg[line].back_line_num = line;
		if (tmp != typeid(statement::EXIT)
				&& tmp != typeid(statement::GOTO)
				&& tmp != typeid(statement::END_FOR))
		{
			auto nxt = it;
			++nxt;
			if (nxt == prog.cend())
				add_edge_to(simple_cfg, line, END_IDX);
			else
				add_edge_to(simple_cfg, line, nxt->first);
		}
		if (tmp == typeid(statement::EXIT))
			add_edge_to(simple_cfg, line, END_IDX);
		else if (tmp == typeid(statement::GOTO))
			add_edge_to(simple_cfg, line,
					static_cast<statement::GOTO&>(*sent).line);
		else if (tmp == typeid(statement::IF))
			add_edge_to(simple_cfg, line,
					static_cast<statement::IF&>(*sent).line);
		else if (tmp == typeid(statement::FOR))
		{
			auto end_for_line
				= static_cast<statement::FOR&>(*sent).end_for_line;
			auto jump_pos = prog.upper_bound(end_for_line);
			if (jump_pos == prog.cend())
				add_edge_to(simple_cfg, line, END_IDX);
			else
				add_edge_to(simple_cfg, line, jump_pos->first);
		}
		else
		{
			assert(tmp == typeid(statement::END_FOR));
			add_edge_to(simple_cfg, line,
					static_cast<statement::END_FOR&>(*sent).for_line);
		}
	}
	for (auto it = simple_cfg.lower_bound(0); it != simple_cfg.end(); ++it)
		if (it->second.in_edge.size() == 0)
			remove_sent(simple_cfg, it->first);
	for (auto it = simple_cfg.lower_bound(0); it != simple_cfg.end(); ++it)
	{
		auto nxt = it;
		++nxt;
		if (nxt == simple_cfg.end())
			break;
		while (it->second.out_edge.size() == 1 &&
				nxt->second.in_edge.size() == 1 &&
				it->second.out_edge.front() == nxt->first)
		{
			it->second.back_line_num = nxt->first;
			it->second.out_edge = nxt->second.out_edge;
			nxt = simple_cfg.erase(nxt);
			if (nxt == simple_cfg.end())
				break;
		}
	}
	// TODO HERE
}
}
#endif
