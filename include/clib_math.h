/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_MATH_H
#define CLIB_MATH_H

#include <stddef.h>

#if defined(USE_SAFE_INTEGER)
/**
 * Check whether 'number' can be exactly represented as an
 * IEEE-754 double precision number without rounding.
 */
#define IS_SAFE_INTEGER(number) \
    (!(((number) < -9007199254740991.0) || ((number) > 9007199254740991.0)))
#else
/* Check whether 'number' is in range [LONG_LONG_MIN ... LONG_LONG_MAX] */
#define IS_SAFE_INTEGER(number) \
    (!(((number) < -9223372036854775807.0) || ((number) > 9223372036854775807)))
#endif

size_t next_pow2(size_t);
unsigned next_size(unsigned);

#endif

