#include <catch2/catch.hpp>
#include "vmmu.hpp"
#include "memory.hpp"

using namespace vmmu;

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

  WORD reads(uint64_t phys_addr)
  {
    return read(phys_addr, WORD());
  }

  void write(uint64_t phys_addr, WORD value)
  {
    mem.write(phys_addr, value);
  }

  bool cmpxchg(uint64_t phys_addr, WORD expected, WORD new_value) override
  {
    if (reads(phys_addr) == expected) {
      write(phys_addr, new_value);
      return true;
    } else {
      return false;
    }
  }

  using operation_type = typename memory<WORD>::operation_type;

  template <typename H>
  void execute_after(operation_type op_type, uint64_t address, H &&async_handler)
  {
    mem.execute_after(op_type, address, std::forward<H>(async_handler));
  }

  size_t count_operations(operation_type op_type, uint64_t address) const
  {
    return mem.count_operations(op_type, address);
  }
};

using test_memory_32 = test_memory<uint32_t>;

// A helper function, because catch2 doesn't understand & as operator.
template <typename WORD, typename WORD2>
static bool is_bit_set(WORD v, WORD2 bit)
{
  return v & bit;
}

} // namespace

TEST_CASE("Disabled paging works", "[translate]")
{
  paging_state const s { RFLAGS_RSVD, 0, 0, 0, 0, 0 };
  test_memory_32 mem;

  SECTION("Translation succeeds without touching memory") {
    auto tlbe_v = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(tlbe_v));

    auto tlbe = std::get<tlb_entry>(tlbe_v);
    CHECK(tlbe.attr().is_w());
    CHECK(tlbe.attr().is_d());
    CHECK_FALSE(tlbe.attr().is_xd());
    CHECK(tlbe.attr().is_u());

    CHECK(tlbe.phys_addr() == 0);
    CHECK(tlbe.linear_addr() == 0);
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
    CHECK(tlbe.linear_addr() == 0);
    CHECK(tlbe.phys_addr() == 0);
    CHECK(tlbe.size() == (4 << 10 /* KiB */));
    CHECK_FALSE(tlbe.attr().is_u());
    CHECK_FALSE(tlbe.attr().is_w());
    CHECK_FALSE(tlbe.attr().is_xd());
  }

  SECTION("4MB large page read-only page is not recognized without CR4.PSE") {
    mem.write(0, uint32_t(PTE_P | PTE_PS));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.size() == (4 << 10 /* KiB */));
  }
}

TEST_CASE("32-bit paging works with PSE", "[translate]")
{
  paging_state const s { RFLAGS_RSVD, CR0_PG, 0, CR4_PSE, 0, 0 };
  test_memory_32 mem;

  SECTION("Self-referencing read-only page") {
    mem.write(0, uint32_t(PTE_P));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.linear_addr() == 0);
    CHECK(tlbe.phys_addr() == 0);
    CHECK(tlbe.size() == (4 << 10 /* KiB */));
  }

  SECTION("4MB large page read-only page is recognized with CR4.PSE") {
    mem.write(0, uint32_t(PTE_P | PTE_PS));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.size() == (4 << 20 /* MiB */));
  }
}

TEST_CASE("Access-once semantics")
{
  paging_state const s { RFLAGS_RSVD, CR0_PG, 0, 0, 0, 0 };
  test_memory_32 mem;

  SECTION("Page table entries are read once when no A/D need to be set") {
    // These two already count as writes. So the write operations below are
    // "off-by-one".
    mem.write(0,      0x1000 | uint32_t(PTE_P | PTE_A));
    mem.write(0x1000, uint32_t(PTE_P | PTE_A | PTE_D));

    auto res = translate({0, linear_memory_op::access_type::WRITE}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    CHECK(mem.count_operations(test_memory_32::operation_type::READ, 0) == 1);
    CHECK(mem.count_operations(test_memory_32::operation_type::WRITE, 0) == 1);

    CHECK(mem.count_operations(test_memory_32::operation_type::READ, 0x1000) == 1);
    CHECK(mem.count_operations(test_memory_32::operation_type::WRITE, 0x1000) == 1);
  }

  SECTION("Page table entries are read twice and written once when A/D bits need to be set") {
    mem.write(0,      0x1000 | uint32_t(PTE_P));
    mem.write(0x1000, uint32_t(PTE_P));

    auto res = translate({0, linear_memory_op::access_type::WRITE}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    CHECK(mem.count_operations(test_memory_32::operation_type::READ, 0) == 2);
    CHECK(mem.count_operations(test_memory_32::operation_type::WRITE, 0) == 2);

    CHECK(mem.count_operations(test_memory_32::operation_type::READ, 0x1000) == 2);
    CHECK(mem.count_operations(test_memory_32::operation_type::WRITE, 0x1000) == 2);
  }
}

TEST_CASE("Failed atomic updates result in retry")
{
  paging_state const s { RFLAGS_RSVD, CR0_PG, 0, 0, 0, 0 };
  test_memory_32 mem;

  SECTION("Failed compare-exchange results in retry") {
    mem.write(0,      0x1000 | uint32_t(PTE_P));
    mem.write(0x1000, 0xA000 | uint32_t(PTE_P));
    mem.write(0x2000, 0xB000 | uint32_t(PTE_P));

    // When the walker read the page directory entry, we switch the entry,
    // before it can set the accessed flag.
    mem.execute_after(test_memory_32::operation_type::READ, 0,
                      [] (auto *m) {
                        m->write(0, 0x2000 | uint32_t(PTE_P));
                      });

    auto res = translate({0, linear_memory_op::access_type::WRITE}, s, &mem);
    REQUIRE(std::holds_alternative<tlb_entry>(res));

    auto tlbe = std::get<tlb_entry>(res);
    CHECK(tlbe.phys_addr() == 0xB000);
  }
}

TEST_CASE("Dirty bit is set correctly")
{
  paging_state const s { RFLAGS_RSVD, CR0_PG | CR0_WP, 0, 0, 0, 0 };
  test_memory_32 mem;

  SECTION("Dirty bit is not set for read") {
    mem.write(0,      0x1000 | uint32_t(PTE_P));
    mem.write(0x1000, uint32_t(PTE_P));

    auto res = translate({0, linear_memory_op::access_type::READ}, s, &mem);
    CHECK(std::holds_alternative<tlb_entry>(res));

    CHECK_FALSE(is_bit_set(mem.reads(0),      PTE_D));
    CHECK_FALSE(is_bit_set(mem.reads(0x1000), PTE_D));
  }

  SECTION("Dirty bit is not set for failed write") {
    mem.write(0,      0x1000 | uint32_t(PTE_P));
    mem.write(0x1000, uint32_t(PTE_P));

    auto res = translate({0, linear_memory_op::access_type::WRITE}, s, &mem);
    CHECK(std::holds_alternative<page_fault_info>(res));

    CHECK_FALSE(is_bit_set(mem.reads(0),      PTE_D));
    CHECK_FALSE(is_bit_set(mem.reads(0x1000), PTE_D));
  }

  SECTION("Dirty bit is set in leaf level for write") {
    mem.write(0,      0x1000 | uint32_t(PTE_P | PTE_W));
    mem.write(0x1000, uint32_t(PTE_P | PTE_W));

    auto res = translate({0, linear_memory_op::access_type::WRITE}, s, &mem);
    CHECK(std::holds_alternative<tlb_entry>(res));

    CHECK_FALSE(is_bit_set(mem.reads(0),      PTE_D));
    CHECK      (is_bit_set(mem.reads(0x1000), PTE_D));
  }
}

// TODO Test ignored bits in CR3.
// TODO Test reserved bits in page table entries (even those that depend on PS bit).
// TODO Test setting A/D bits, D bits should only be set if translation succeeds
