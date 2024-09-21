/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_MATH_H
#define CLIB_MATH_H

#include <stddef.h>

int is_safe_integer(double);
size_t next_pow2(size_t);
unsigned next_size(unsigned);

#endif

