#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <variant>

#include "vmmu_utilities.hpp"

#if defined(__clang__)
#define __VMMU_UBSAN_NO_UNSIGNED_OVERFLOW__ \
  __attribute__((no_sanitize("unsigned-integer-overflow")))
#else
#define __VMMU_UBSAN_NO_UNSIGNED_OVERFLOW__
#endif

namespace vmmu
{
// Architecture definitions

enum {
  RFLAGS_RSVD = uint64_t(1) << 1,
  RFLAGS_AC = uint64_t(1) << 18,

  CR0_WP = uint64_t(1) << 16,
  CR0_PG = uint64_t(1) << 31,

  CR4_PSE = uint64_t(1) << 4,
  CR4_PAE = uint64_t(1) << 5,
  CR4_PGE = uint64_t(1) << 7,
  CR4_PCIDE = uint64_t(1) << 17,
  CR4_SMEP = uint64_t(1) << 20,
  CR4_SMAP = uint64_t(1) << 21,
  CR4_PKE = uint64_t(1) << 22,

  EFER_LME = uint64_t(1) << 8,
  EFER_NXE = uint64_t(1) << 11,

  PTE_P = uint64_t(1) << 0,
  PTE_W = uint64_t(1) << 1,
  PTE_U = uint64_t(1) << 2,
  PTE_A = uint64_t(1) << 5,
  PTE_D = uint64_t(1) << 6,
  PTE_PS = uint64_t(1) << 7,
  PTE_XD = uint64_t(1) << 63,

  EC_P = uint64_t(1) << 0,     // Page was present
  EC_W = uint64_t(1) << 1,     // Access was a write
  EC_U = uint64_t(1) << 2,     // Access was user access
  EC_RSVD = uint64_t(1) << 3,  // PTE had reserved bit set
  EC_I = uint64_t(1) << 4,     // Access was instruction fetch
};

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

// Contains CPU state necessary for page table walks. See Intel SDM Vol. 3 4.1
// "Paging Modes and Control Bits".
class paging_state
{
  uint64_t cr3;
  std::array<uint64_t, 4> pdpte;

  bool cr0_wp, cr0_pg;

  bool cr4_pse, cr4_pae;
  bool cr4_smep, cr4_smap;

  bool efer_lme, efer_nxe;

  bool rflags_ac;

  bool cpl_is_supervisor;

public:
  uint64_t get_pdpte(size_t i) const
  {
    fast_assert(i < pdpte.size());
    return pdpte[i];
  }

  uint64_t get_cr3() const { return cr3; }

  bool get_cr0_wp() const { return cr0_wp; }
  bool get_cr0_pg() const { return cr0_pg; }

  bool get_cr4_pse() const { return cr4_pse; }
  bool get_cr4_pae() const { return cr4_pae; }
  bool get_cr4_smep() const { return cr4_smep; }
  bool get_cr4_smap() const { return cr4_smap; }

  bool get_efer_nxe() const { return efer_nxe; }
  bool get_rflags_ac() const { return rflags_ac; }

  // Compute the paging mode as per Intel SDM Vol. 3 4.1.1 "Three Paging Modes"
  // (which are actually four). The conditions are written slightly verbose to
  // match 1:1 with the manual.
  paging_mode get_paging_mode() const
  {
    if (not cr0_pg)
      return paging_mode::PHYS;

    if (cr0_pg and not cr4_pae)
      return paging_mode::PM32;

    if (cr0_pg and cr4_pae and not efer_lme)
      return paging_mode::PM32_PAE;

    if (cr0_pg and cr4_pae and efer_lme)
      return paging_mode::PM64_4LEVEL;

    unreachable();
  }

  // This returns whether the CPL indicates supervisor mode. This is unrelated
  // to implicit supervisor accesses.
  bool is_supervisor() const { return cpl_is_supervisor; }

  paging_state() = delete;

  paging_state(uint64_t rflags_,
               uint64_t cr0_,
               uint64_t cr3_,
               uint64_t cr4_,
               uint64_t efer_,
               unsigned cpl_,
               decltype(pdpte) const &pdpte_ = {})
      : cr3(cr3_),
        pdpte(pdpte_),
        cr0_wp(cr0_ & CR0_WP),
        cr0_pg(cr0_ & CR0_PG),
        cr4_pse(cr4_ & CR4_PSE),
        cr4_pae(cr4_ & CR4_PAE),
        cr4_smep(cr4_ & CR4_SMEP),
        cr4_smap(cr4_ & CR4_SMAP),
        efer_lme(efer_ & EFER_LME),
        efer_nxe(efer_ & EFER_NXE),
        rflags_ac(rflags_ & RFLAGS_AC),
        cpl_is_supervisor(cpl_ != 3)
  {
    fast_assert(cpl_ <= 3);
  }
};

// A wrapper for TLB entry permissions.
class tlb_attr
{
  // Stores PTE_W, PTE_U, PTE_XD, and PTE_D. The last two are stored inverted to
  // allow for combining attributes with a single AND operation.
  uint64_t pte;

public:
  bool is_w() const { return pte & PTE_W; }
  bool is_u() const { return pte & PTE_U; }
  bool is_xd() const { return ~pte & PTE_XD; }
  bool is_d() const { return ~pte & PTE_D; }

  void set_d() { pte &= ~PTE_D; }

  static tlb_attr combine(tlb_attr const &a, tlb_attr const &b)
  {
    tlb_attr attr;

    attr.pte = a.pte & b.pte;

    return attr;
  }

  // Everything is allowed and the entry is marked dirty so writes don't trigger
  // page table walks.
  static tlb_attr no_paging() { return tlb_attr {PTE_W | PTE_U | PTE_D}; }

  explicit tlb_attr(uint64_t pte_) : pte(pte_ ^ (PTE_D | PTE_XD)) {}

  tlb_attr(bool w_, bool u_, bool xd_, bool d_)
      : tlb_attr(PTE_W * w_ | PTE_U * u_ | PTE_XD * xd_ | PTE_D * d_)
  {
  }

  tlb_attr() : tlb_attr(PTE_W | PTE_U) {}
};

// Wrap all data we need to know about the linear memory access that triggered a
// page walk.
struct linear_memory_op {
  uint64_t linear_addr;

  enum class access_type : uint8_t { READ, WRITE, EXECUTE };
  access_type type;

  // Implicit accesses are accesses, such as reading the GDT, which should
  // always be treated as supervisor accesses.
  enum class supervisor_type : uint8_t { IMPLICIT, EXPLICIT };
  supervisor_type sv_type;

  bool is_write() const { return type == access_type::WRITE; }
  bool is_data_read() const { return type == access_type::READ; }
  bool is_instruction_fetch() const { return type == access_type::EXECUTE; }

  bool is_implicit_supervisor() const { return sv_type == supervisor_type::IMPLICIT; }

  linear_memory_op() = delete;
  linear_memory_op(uint64_t linear_addr_,
                   access_type type_,
                   supervisor_type sv_type_ = supervisor_type::EXPLICIT)
      : linear_addr(linear_addr_), type(type_), sv_type(sv_type_)
  {
    fast_assert(not(is_implicit_supervisor() and is_instruction_fetch()));
  }
};

// A TLB entry for an power-of-2 naturally aligned linear memory region.
class tlb_entry
{
  uint64_t linear_addr_;
  uint64_t phys_addr_;
  tlb_attr attr_;
  uint8_t size_bits_;

public:
  uint64_t linear_addr() const { return linear_addr_; }
  uint64_t phys_addr() const { return phys_addr_; }
  tlb_attr const &attr() const { return attr_; }
  tlb_attr &attr() { return attr_; }
  uint64_t size() const { return 1ULL << size_bits_; }

  uint64_t match_mask() const { return ~(size() - 1); }

  std::optional<uint64_t> translate(uint64_t la) const
  {
    uint64_t mask = match_mask();

    if ((la & mask) == linear_addr())
      return (la & ~mask) | phys_addr();

    return {};
  }

  // Returns true, if this TLB entry translates the given operation in the
  // current paging mode. See Intel SDM Vol 3 4.6.1 "Determination of Access
  // Rights" for details.
  bool allows(linear_memory_op const &op, paging_state const &state) const;

  // For non-paged mode, we create a TLB that covers everything and allows
  // everything.
  static tlb_entry no_paging() { return {0, 0, 63, tlb_attr::no_paging()}; }

  tlb_entry() = delete;

  tlb_entry(uint64_t linear_addr, uint64_t phys_addr, uint8_t size_bits, tlb_attr attr)
      : linear_addr_(linear_addr), phys_addr_(phys_addr), attr_(attr), size_bits_(size_bits)
  {
    fast_assert((~match_mask() & this->phys_addr()) == 0);
    fast_assert(size_bits_ < 64);
  }
};

// The interface for physical memory access.
class abstract_memory
{
public:
  virtual uint64_t read(uint64_t phys_addr, uint64_t dummy) = 0;
  virtual uint32_t read(uint64_t phys_addr, uint32_t dummy) = 0;

  virtual bool cmpxchg(uint64_t phys_addr, uint32_t expected, uint32_t new_value) = 0;
  virtual bool cmpxchg(uint64_t phys_addr, uint64_t expected, uint64_t new_value) = 0;

  virtual ~abstract_memory() {}
};

struct page_fault_info {
  uint64_t cr2;
  uint32_t error_code;

  page_fault_info() = delete;
  page_fault_info(uint64_t cr2_, uint32_t error_code_) : cr2(cr2_), error_code(error_code_) {}
};

using translate_result = std::variant<std::monostate, tlb_entry, page_fault_info>;

// Translate a linear memory access given a state of the virtual CPU.
//
// Will return either a TLB entry that translates the operation and where it is
// also guaranteed that the operation is allowed, or it returns page fault
// information.
translate_result translate(linear_memory_op const &op,
                           paging_state const &state,
                           abstract_memory *memory);

// A very primitive fully associative TLB.
//
// Entries are inserted in FIFO order and we look through all cached entries to
// find a match.
template <size_t SIZE>
class tlb
{
  size_t pos_ = 0;

  static_assert(SIZE > 1);
  std::array<std::optional<tlb_entry>, SIZE> entries_;

public:
  // Reset the TLB to its pristine (empty) state.
  void clear() { *this = {}; }

  // This method is semantically identical to the function with the same name
  // above. It just caches its results in the TLB.
  //
  // TODO Write tests.
  translate_result __VMMU_UBSAN_NO_UNSIGNED_OVERFLOW__ translate(linear_memory_op const &op,
                                                                 paging_state const &state,
                                                                 abstract_memory *memory)
  {
    for (size_t i = 0; i < entries_.size(); i++) {
      auto const &entry = entries_[(pos_ + i) % entries_.size()];

      if (entry and entry->translate(op.linear_addr) and entry->allows(op, state))
        return *entry;
    }

    auto res = ::vmmu::translate(op, state, memory);

    if (std::holds_alternative<tlb_entry>(res)) {
      entries_[--pos_ % entries_.size()] = std::get<tlb_entry>(res);
    }

    return res;
  }
};

}  // namespace vmmu
