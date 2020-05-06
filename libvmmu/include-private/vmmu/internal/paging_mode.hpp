#pragma once

#include <vmmu/vmmu.hpp>
#include <vmmu/internal/compiler.hpp>

namespace vmmu::internal
{
enum class paging_mode {
  // Paging is disabled.
  PHYS,

  // Classic 32-bit paging.
  PM32,

  // 32-bit mode with 64-bit page tables.
  PM32_PAE,

  // 4-level 64-bit paging,
  PM64_4LEVEL,
};

// Compute the paging mode as per Intel SDM Vol. 3 4.1.1 "Three Paging Modes"
// (which are actually four). The conditions are written slightly verbose to
// match 1:1 with the manual.
inline paging_mode get_paging_mode(paging_state const &s)
{
  if (not s.get_cr0_pg())
    return paging_mode::PHYS;

  if (s.get_cr0_pg() and not s.get_cr4_pae())
    return paging_mode::PM32;

  if (s.get_cr0_pg() and s.get_cr4_pae() and not s.get_efer_lme())
    return paging_mode::PM32_PAE;

  if (s.get_cr0_pg() and s.get_cr4_pae() and s.get_efer_lme())
    return paging_mode::PM64_4LEVEL;

  unreachable();
}

}  // namespace vmmu::internal
