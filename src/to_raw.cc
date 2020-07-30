#include "to_raw.hpp"
namespace to_raw
{
raw_prog to_raw_prog(const link::linked_prog &code)
{
	raw_prog ret;
	for (const auto &inst : code)
	{
		switch (inst.op)
		{
			// TODO
		}
	}
	return ret;
}
void print_raw_prog(std::ostream &os, const raw_prog &prog)
{
	const auto &os_flag = os.flags();
	os << std::hex;
	os << "@00000000\n";
	for (size_t i = 0; i < prog.size(); ++i)
	{
		os << unsigned(prog[i]);
		if ((i + 1) % 8 == 0)
			os << '\n';
		else
			os << ' ';
	}
	os.flags(os_flag);
}
}
