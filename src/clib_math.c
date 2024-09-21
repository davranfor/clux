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
size_t next_pow2(size_t number)
{
    number -= 1;
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    if (sizeof(number) > 4)
    {
        number |= number >> 32;
    }
    number += 1;
    return number;
}

/* Returns the next power of two if size is a power of two, size otherwise */
unsigned next_size(unsigned size)
{
    if ((size & (size - 1)) == 0)
    {
        return size == 0 ? 1 : size << 1;
    }
    return size;
}

