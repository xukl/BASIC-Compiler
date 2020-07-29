#include "basic_block.hpp"
#include <typeinfo>
#include <map>
#include <vector>
#include <set>
#include <cassert>
#include <iostream>
#include <cstring>
namespace basic_block
{
struct simple_block
{
	std::vector<statement::line_num> lines;
	int in_edge_cnt, out_edge_cnt;
	int out_edge[2];
};
using num_cfg_type = std::map<int, simple_block>;
inline void add_edge_to(num_cfg_type &m, int u, int v)
{
	m[u].out_edge[m[u].out_edge_cnt++] = v;
	++m[v].in_edge_cnt;
}
num_cfg_type::iterator
remove_sent(num_cfg_type &m, int x, const program_type &prog)
{
	if (x != statement::additional_exit_line)
		std::clog << "Warning: unreachable code at line " << x << std::endl;
	if (typeid(*(prog.at(x))) == typeid(statement::LET))
		std::clog << "Warning: remove unreachable LET. It may cause error."
			<< std::endl;
	for (int i = 0; i < m[x].out_edge_cnt; ++i)
	{
		auto v = m[x].out_edge[i];
		auto &v_in_cnt = m[v].in_edge_cnt;
		if (--v_in_cnt == 0)
			remove_sent(m, v, prog);
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
		ret[line].lines.push_back(line);
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
			it = remove_sent(ret, it->first, prog);
		else
			++it;
	for (auto it = ret.lower_bound(0); it != ret.end(); ++it)
	{
		auto &it_node = it->second;
		while (it_node.out_edge_cnt == 1)
		{
			auto nxt_id = it_node.out_edge[0];
			auto nxt = ret[nxt_id];
			if (nxt_id == END_IDX || nxt.in_edge_cnt != 1)
				break;
			it_node.out_edge_cnt = nxt.out_edge_cnt;
			memcpy(it_node.out_edge, nxt.out_edge, sizeof(it_node.out_edge));
			if (typeid(*(prog.at(it_node.lines.back())))
					== typeid(statement::GOTO))
				it_node.lines.pop_back();
			it_node.lines.insert(it_node.lines.end(),
					nxt.lines.begin(), nxt.lines.end());
			ret.erase(nxt_id);
		}
	}
	return ret;
}
cfg_type gen_cfg(const program_type &prog)
{
	const auto &simple_cfg = gen_num_cfg(prog);
	cfg_type ret;
	for (auto &&num_cfg_node = simple_cfg.lower_bound(0);
			num_cfg_node != simple_cfg.cend();
			++num_cfg_node)
	{
		std::vector<std::unique_ptr<statement::statement>> block_statement;
		for (const auto &line : num_cfg_node->second.lines)
			block_statement.push_back(prog.at(line)->deep_copy());
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
