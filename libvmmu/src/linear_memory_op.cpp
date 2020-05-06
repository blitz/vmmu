#include <cassert>
#include <vmmu/vmmu.hpp>

vmmu::linear_memory_op::linear_memory_op(uint64_t linear_addr_,
                                         access_type type_,
                                         supervisor_type sv_type_)
    : linear_addr(linear_addr_), type(type_), sv_type(sv_type_)
{
  assert(not(is_implicit_supervisor() and is_instruction_fetch()));
}
