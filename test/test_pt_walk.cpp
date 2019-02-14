#include <catch2/catch.hpp>
#include "pt_walk.hpp"
#include "memory.hpp"

namespace {

// Default implementations for abstract methods that just abort.
class test_memory_base : public abstract_memory {
public:

  uint32_t read(uint64_t phys_addr, uint32_t) override { __builtin_trap(); }
  uint64_t read(uint64_t phys_addr, uint64_t) override { __builtin_trap(); }

  bool cmpxchg(uint64_t phys_addr, uint64_t expected, uint64_t new_value) override { __builtin_trap(); }
  bool cmpxchg(uint64_t phys_addr, uint32_t expected, uint32_t new_value) override { __builtin_trap(); }
};

template <typename WORD>
class test_memory final : public test_memory_base {

  memory<WORD> mem;

public:

  WORD read(uint64_t phys_addr, WORD) override
  {
    return mem.read(phys_addr);
  }

  void write(uint64_t phys_addr, WORD value)
  {
    mem.write(phys_addr, value);
  }

  bool cmpxchg(uint64_t phys_addr, WORD expected, WORD new_value) override
  {
    if (read(phys_addr, WORD()) == expected) {
      write(phys_addr, new_value);
      return true;
    } else {
      return false;
    }
  }
};

using test_memory_32 = test_memory<uint32_t>;

} // namespace

TEST_CASE("Disabled paging works", "[translate]")
{
  paging_state const s { RFLAGS_RSVD, 0, 0, 0, 0, 0 };
  test_memory_32 mem;

  SECTION("Translation succeeds without touching memory") {
    auto tlbe_v = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(tlbe_v));

    auto tlbe = std::get<tlb_entry>(tlbe_v);
    CHECK(tlbe.attr.is_w());
    CHECK(tlbe.attr.is_d());
    CHECK_FALSE(tlbe.attr.is_xd());
    CHECK(tlbe.attr.is_u());

    CHECK(tlbe.phys_addr == 0);
    CHECK(tlbe.linear_addr == 0);
    CHECK(tlbe.size() > (1ULL << 30));
  }
}

TEST_CASE("32-bit paging works without PSE", "[translate]")
{
  paging_state const s { RFLAGS_RSVD, CR0_PG, 0, 0, 0, 0 };
  test_memory_32 mem;

  SECTION("No translation for non-present PDE") {
    mem.write(0, uint32_t(0));
    REQUIRE(std::holds_alternative<page_fault_info>(translate({0, linear_memory_op::access_type::READ}, s, &mem)));
  }

  SECTION("Self-referencing read-only page") {
    mem.write(0, uint32_t(PTE_P));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.linear_addr == 0);
    CHECK(tlbe.phys_addr == 0);
    CHECK(tlbe.size() == (4 << 10 /* KiB */));
    CHECK_FALSE(tlbe.attr.is_u());
    CHECK_FALSE(tlbe.attr.is_w());
    CHECK_FALSE(tlbe.attr.is_xd());
  }

  SECTION("4MB large page read-only page is not recognized without CR4.PSE") {
    mem.write(0, uint32_t(PTE_P | PTE_PS));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.size() == (4 << 10 /* KiB */));
  }
}

TEST_CASE("32-bit paging works PSE", "[translate]")
{
  paging_state const s { RFLAGS_RSVD, CR0_PG, 0, CR4_PSE, 0, 0 };
  test_memory_32 mem;

  SECTION("Self-referencing read-only page") {
    mem.write(0, uint32_t(PTE_P));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.linear_addr == 0);
    CHECK(tlbe.phys_addr == 0);
    CHECK(tlbe.size() == (4 << 10 /* KiB */));
  }

  SECTION("4MB large page read-only page is recognized without CR4.PSE") {
    mem.write(0, uint32_t(PTE_P | PTE_PS));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.size() == (4 << 20 /* MiB */));
  }

}

// TODO Test ignored bits in CR3.
// TODO Test reserved bits in page table entries (even those that depend on PS bit).
// TODO Test setting A/D bits, D bits should only be set if translation succeeds
