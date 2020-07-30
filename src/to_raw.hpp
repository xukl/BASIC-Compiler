#ifndef TO_RAW_HPP
#define TO_RAW_HPP
#include "link.hpp"
#include <vector>
#include <cstdint>
namespace to_raw
{
using raw_prog = std::vector<uint8_t>;
raw_prog to_raw_prog(const link::linked_prog &code);
void print_raw_prog(std::ostream &os, const raw_prog &prog);
}
#endif
