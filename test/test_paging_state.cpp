#include <catch2/catch.hpp>

#include "vmmu.hpp"

using namespace vmmu;

TEST_CASE("Physical paging is recognized", "[paging_state]")
{
  paging_state const s {RFLAGS_RSVD, 0, 0, 0, 0, 0};
  REQUIRE(s.get_paging_mode() == paging_mode::PHYS);
}

TEST_CASE("Ordinary 32-bit paging is recognized", "[paging_state]")
{
  paging_state const s {RFLAGS_RSVD, CR0_PG, 0, 0, 0, 0};
  REQUIRE(s.get_paging_mode() == paging_mode::PM32);
}

TEST_CASE("32-bit PAE paging is recognized", "[paging_state]")
{
  paging_state const s {RFLAGS_RSVD, CR0_PG, 0, CR4_PAE, 0, 0};
  REQUIRE(s.get_paging_mode() == paging_mode::PM32_PAE);
}

TEST_CASE("4-level 64-bit paging is recognized", "[paging_state]")
{
  paging_state s {RFLAGS_RSVD, CR0_PG, 0, CR4_PAE, EFER_LME, 0};
  REQUIRE(s.get_paging_mode() == paging_mode::PM64_4LEVEL);
}
