#include "link.hpp"
#include <map>
namespace link
{
using namespace inst;
linked_prog link(const translate::obj_code &obj)
{
	linked_prog ret;
	std::map<int, int> block_pc_map;
	std::map<int, int> block_jump_pos;
	ret.push_back(instruction{inst_op::LUI, 0, 0, 0x20 << 12, sp});
	for (const auto &[block_id, block] : obj)
	{
		block_pc_map[block_id] = ret.size() * 4;
		ret.insert(ret.end(),
				block.instructions.begin(), block.instructions.end());
		block_jump_pos[block_id] = ret.size();
		if (block.condition == nullptr)
		{
			if (block.jump_true != basic_block::END_IDX)
				ret.insert(ret.end(), 2, inst_NOP);
		}
		else
			ret.insert(ret.end(), 5, inst_NOP);
	}
	for (const auto &[block_id, block] : obj)
	{
		auto pos = block_jump_pos[block_id];
		if (block.condition == nullptr)
		{
			if (block.jump_true != basic_block::END_IDX)
			{
				int delta_pc = block_pc_map[block.jump_true] - pos * 4;
				auto &&[high, low] = split_int32(delta_pc);
				ret[pos++] = instruction{inst_op::AUIPC, 0, 0, high, t0};
				ret[pos++] = instruction{inst_op::JALR, t0, 0, low, zero};
			}
		}
		else
		{
			ret[pos++] = instruction{inst_op::BEQ, a0, zero, 3 * 4, 0};
			auto &&[h1, l1] =
				split_int32(block_pc_map[block.jump_true] - pos * 4);
			ret[pos++] = instruction{inst_op::AUIPC, 0, 0, h1, t0};
			ret[pos++] = instruction{inst_op::JALR, t0, 0, l1, zero};
			auto &&[h2, l2] =
				split_int32(block_pc_map[block.jump_false] - pos * 4);
			ret[pos++] = instruction{inst_op::AUIPC, 0, 0, h2, t0};
			ret[pos++] = instruction{inst_op::JALR, t0, 0, l2, zero};
		}
	}
	return ret;
}
void print_linked_code(std::ostream &os, const linked_prog &code)
{
	const auto &os_flag = os.flags();
	for (size_t i = 0; i < code.size(); ++i)
	{
		os << std::hex << 4*i << ":\t ";
		os.flags(os_flag);
		os << code[i] << std::endl;
	}
}
}
