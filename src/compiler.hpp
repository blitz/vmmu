#pragma once

#if defined(__clang__)
# define __VMMU_UBSAN_NO_UNSIGNED_OVERFLOW__ __attribute__((no_sanitize("unsigned-integer-overflow")))
#else
# define __VMMU_UBSAN_NO_UNSIGNED_OVERFLOW__
#endif
