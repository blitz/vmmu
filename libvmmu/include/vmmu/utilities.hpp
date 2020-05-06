#pragma once

namespace vmmu
{
// Utilities

inline bool likely(bool c)
{
  return __builtin_expect(c, true);
}
inline bool unlikely(bool c)
{
  return __builtin_expect(c, false);
}

[[noreturn]] inline void unreachable()
{
  __builtin_unreachable();
}

inline void fast_assert(bool c)
{
  if (unlikely(not c))
    __builtin_trap();
}

}  // namespace vmmu
