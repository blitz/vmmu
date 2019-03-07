#include "pt_walk.hpp"

using namespace vmmu;

// I've tried to make the code match the written description in the manual 1:1.
// Once all cases are covered by testing, we can dare to simplify.
bool vmmu::tlb_entry::allows(linear_memory_op const &op, paging_state const &state) const
{
  auto mode = state.get_paging_mode();

  // No permission checking without paging.
  if (mode == paging_mode::PHYS)
    return true;

  if (op.is_implicit_supervisor() or state.is_supervisor()) {
    // For supervisor-mode accesses:

    // Data may be read (implicitly or explicitly) from any supervisor-mode address.
    if (op.is_data_read() and not attr().is_u())
      return true;

    // Data reads from user-mode pages.
    if (op.is_data_read() and attr().is_u()) {
      // Access rights depend on the value of CR4.SMAP:

      // If CR4.SMAP = 0, data may be read from any user-mode address with a
      // protection key for which read access is permitted.
      if (not state.cr4_smap)
        return true;

      // If CR4.SMAP = 1, access rights depend on the value of EFLAGS.AC and
      // whether the access is implicit or explicit:
      if (state.cr4_smap) {
        // If EFLAGS.AC = 1 and the access is explicit, data may be read from
        // any user-mode address with a protection key for which read access
        // is permitted.
        if (state.rflags_ac and not op.is_implicit_supervisor())
          return true;

        // If EFLAGS.AC = 0 or the access is implicit, data may not be read
        // from any user-mode address.
        if (not state.rflags_ac or op.is_implicit_supervisor())
          return false;
      }

      unreachable();
    }

    // Data writes to supervisor-mode addresses.
    if (op.is_write() and not attr().is_u()) {
      // Access rights depend on the value of CR0.WP:

      // If CR0.WP = 0, data may be written to any supervisor-mode address.
      if (not state.cr0_wp)
        return true;

      // If CR0.WP = 1, data may be written to any supervisor-mode address
      // with a translation for which the R/W flag (bit 1) is 1 in every
      // paging-structure entry controlling the translation; data may not be
      // written to any supervisor-mode address with a translation for which
      // the R/W flag is 0 in any paging-structure entry controlling the
      // translation.
      if (state.cr0_wp)
        return attr().is_w();

      unreachable();
    }

    // Data writes to user-mode addresses.
    if (op.is_write() and attr().is_u()) {
      // Access rights depend on the value of CR0.WP:

      // If CR0.WP = 0, access rights depend on the value of CR4.SMAP:
      if (not state.cr0_wp) {
        // If CR4.SMAP = 0, data may be written to any user-mode address with
        // a protection key for which write access is permitted.
        if (not state.cr4_smap)
          return true;

        // If CR4.SMAP = 1, access rights depend on the value of EFLAGS.AC and
        // whether the access is implicit or explicit.
        if (state.cr4_smap) {
          // If EFLAGS.AC = 1 and the access is explicit, data may be written
          // to any user-mode address with a protection key for which write
          // access is permitted.
          if (state.rflags_ac and not op.is_implicit_supervisor())
            return true;

          // If EFLAGS.AC = 0 or the access is implicit, data may not be
          // written to any user-mode address.
          if (not state.rflags_ac or op.is_implicit_supervisor())
            return false;

          unreachable();
        }
      }

      // If CR0.WP = 1, access rights depend on the value of CR4.SMAP:
      if (state.cr0_wp) {
        // If CR4.SMAP = 0, data may be written to any user-mode address with
        // a translation for which the R/W flag is 1 in every paging-structure
        // entry controlling the translation and with a protection key for
        // which write access is permitted; Data may not be written to any
        // user-mode address with a translation for which the R/W flag is 0 in
        // any paging-structure entry controlling the translation.
        if (not state.cr4_smap)
          return attr().is_w();

        // If CR4.SMAP = 1, access rights depend on the value of EFLAGS.AC and
        // whether the access is implicit or explicit:
        if (state.cr4_smap) {

          // If EFLAGS.AC = 1 and the access is explicit, data may be written
          // to any user-mode address with a translation for which the R/W
          // flag is 1 in every paging-structure entry controlling the
          // translation and with a protection key for which write access is
          // permitted; data may not be written to any user-mode address with
          // a translation for which the R/W flag is 0 in any paging-structure
          // entry controlling the translation.
          if (state.rflags_ac and not op.is_implicit_supervisor())
            return attr().is_w();

          // If EFLAGS.AC = 0 or the access is implicit, data may not be
          // written to any user-mode address.
          if (not state.rflags_ac or op.is_implicit_supervisor())
            return false;

          unreachable();
        }
      }
    }

    // Instruction fetches from supervisor-mode addresses.
    if (op.is_instruction_fetch() and not attr().is_u()) {
      // For 32-bit paging or if IA32_EFER.NXE = 0, instructions may be
      // fetched from any supervisor-mode address.
      if (mode == paging_mode::PM32 or not state.efer_nxe)
        return true;

      // For PAE paging or 4-level paging with IA32_EFER.NXE = 1, instructions
      // may be fetched from any supervisor-mode address with a translation for
      // which the XD flag (bit 63) is 0 in every paging-structure entry
      // controlling the translation; instructions may not be fetched from any
      // supervisor-mode address with a translation for which the XD flag is 1
      // in any paging-structure entry controlling the translation.
      return not attr().is_xd();
    }

    // Instruction fetches from user-mode addresses.
    if (op.is_instruction_fetch() and attr().is_u()) {
      // Access rights depend on the values of CR4.SMEP:

      // If CR4.SMEP = 0, access rights depend on the paging mode and the
      // value of IA32_EFER.NXE:
      if (not state.cr4_smep) {
        // For 32-bit paging or if IA32_EFER.NXE = 0, instructions may be
        // fetched from any user-mode address.
        if (mode == paging_mode::PM32 or not state.efer_nxe)
          return true;

        // For PAE paging or 4-level paging with IA32_EFER.NXE = 1, instructions
        // may be fetched from any user-mode address with a translation for
        // which the XD flag is 0 in every paging-structure entry controlling
        // the translation; instructions may not be fetched from any user-mode
        // address with a translation for which the XD flag is 1 in any
        // paging-structure entry controlling the translation.
        return not attr().is_xd();
      }

      // If CR4.SMEP = 1, instructions may not be fetched from any user-mode address.
      if (state.cr4_smep)
        return false;

      unreachable();
    }

    unreachable();
  }

  // For—user-mode accesses:

  // Data reads.
  if (op.is_data_read()) {
    // Access rights depend on the mode of the linear address:

    // Data may be read from any user-mode address with a protection key for
    // which read access is permitted.
    //
    // Data may not be read from any supervisor-mode address.
    return attr().is_u();
  }

  // Data writes.
  if (op.is_write()) {
    // Access rights depend on the mode of the linear address:

    // Data may be written to any user-mode address with a translation for which
    // the R/W flag is 1 in every paging-structure entry controlling the
    // translation and with a protection key for which write access is
    // permitted.
    if (attr().is_u())
      return attr().is_w();

    // Data may not be written to any supervisor-mode address.
    if (not attr().is_u())
      return false;

    unreachable();
  }

  // Instruction fetches.
  if (op.is_instruction_fetch()) {
    // Access rights depend on the mode of the linear address, the paging mode,
    // and the value of IA32_EFER.NXE:

    // Instructions may not be fetched from any supervisor-mode address.
    if (not attr().is_u())
      return false;

    // For 32-bit paging or if IA32_EFER.NXE = 0, instructions may be fetched
    // from any user-mode address.
    if (mode == paging_mode::PM32 or not state.efer_nxe)
      return true;

    // For PAE paging or 4-level paging with IA32_EFER.NXE = 1, instructions may
    // be fetched from any user-mode address with a translation for which the XD
    // flag is 0 in every paging-structure entry controlling the translation.
    return not attr().is_xd();
  }

  // We forgot to handle a case.
  fast_assert(false);
  return false;
}
