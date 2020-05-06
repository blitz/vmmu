#include <cassert>
#include <vmmu/vmmu.hpp>

vmmu::paging_state::paging_state(uint64_t rflags_,
                                 uint64_t cr0_,
                                 uint64_t cr3_,
                                 uint64_t cr4_,
                                 uint64_t efer_,
                                 unsigned cpl_,
                                 decltype(vmmu::paging_state::pdpte) const &pdpte_)
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
  assert(cpl_ <= 3);
}
