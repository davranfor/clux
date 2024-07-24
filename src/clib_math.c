/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <math.h>
#include "clib_math.h"

int is_safe_integer(double number)
{
    int exponent;
    double mantissa = frexp(number, &exponent);
 
    if ((exponent > -53) && (exponent < 53))
    {
        return 1;
    }
    return ((exponent == -53) || (exponent == 53))
        && ((mantissa > -1.0) && (mantissa < 1.0));
} 

/* Returns the smallest power of two that is greater than or equal to size */
size_t next_pow2(size_t size)
{
    size -= 1;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size += 1;
    return size;
}

