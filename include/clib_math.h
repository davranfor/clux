/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_MATH_H
#define CLIB_MATH_H

#include <stddef.h>

/**
 * Check whether 'number' can be exactly represented as an
 * IEEE-754 double precision number without rounding.
 */
#define IS_SAFE_INTEGER(number) \
    (!(((number) < -9007199254740991.0) || ((number) > 9007199254740991.0)))

#define next_size(size) _Generic((size), unsigned: next_uint, size_t: next_ulong)(size)

int rrand(int);
int rand_bytes(unsigned char *, size_t);
size_t next_pow2(size_t);
unsigned next_uint(unsigned);
size_t next_ulong(size_t);

#endif

