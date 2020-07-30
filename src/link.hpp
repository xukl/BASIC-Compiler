#ifndef LINK_HPP
#define LINK_HPP
#include "translate.hpp"
namespace link
{
using namespace inst;
using linked_prog = std::vector<instruction>;
linked_prog link(const translate::obj_code &obj);
void print_linked_code(std::ostream &os, const linked_prog &code);
}
#endif
