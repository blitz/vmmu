#pragma once

namespace vmmu::internal
{

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

}  // namespace vmmu
