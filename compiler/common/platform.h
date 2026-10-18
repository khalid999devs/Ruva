#ifndef RUVA_PLATFORM_H
#define RUVA_PLATFORM_H

#include <float.h>
#include <limits.h>
#include <stdint.h>

#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__)
#error "Ruva requires floating-point builds without fast-math assumptions"
#endif

_Static_assert(CHAR_BIT == 8, "Ruva requires eight-bit bytes");
_Static_assert(sizeof(int32_t) == 4, "Ruva requires a signed 32-bit integer");
_Static_assert(sizeof(int64_t) == 8, "Ruva requires a signed 64-bit integer");
_Static_assert(sizeof(double) == 8 && FLT_RADIX == 2 && DBL_MANT_DIG == 53
               && DBL_MIN_EXP == -1021 && DBL_MAX_EXP == 1024,
               "Ruva requires binary64 double");

#endif
