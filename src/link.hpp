#ifndef LINK_HPP
#define LINK_HPP
#include "translate.hpp"
namespace link
{
using namespace inst;
std::vector<instruction> link(const translate::obj_code &obj);
void print_linked_code(std::ostream &os, const std::vector<instruction> &code);
}
#endif
