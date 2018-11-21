#pragma once

// Utilities

inline bool likely(bool c)   { return __builtin_expect(c, true);  }
inline bool unlikely(bool c) { return __builtin_expect(c, false); }

[[noreturn]] inline void unreachable() { __builtin_unreachable(); }

inline void fast_assert(bool c)
{
  if (unlikely(not c)) __builtin_trap();
}

// Bit manipulation

template<int HI, int LO>
struct bit_range {
  static_assert(HI >= LO);

  static const int hi = HI;
  static const int lo = LO;

  // Extracts bits hi to lo from value.
  static uint64_t extract(uint64_t value)
  {
    return (value >> LO) & ((uint64_t(1) << (1 + HI - LO)) - 1);
  }

  // Return a mask that covers the given bit range.
  static uint64_t mask()
  {
    return ((uint64_t(1) << (1 + HI - LO)) - 1) << LO;
  }

  // Masks everything in the given value except the bits from hi to lo.
  static uint64_t extract_no_shift(uint64_t value)
  {
    return value & mask();
  }

};
