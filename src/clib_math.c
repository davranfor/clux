/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "clib_math.h"

/* Returns the smallest power of 2 that is greater than or equal to size */
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

/* Returns the next power of 2 if 'size' is a power of 2, 'size' otherwise */
unsigned next_size(unsigned size)
{
    if (size == 0)
    {
        return 1;
    }
    if ((size & (size - 1)) == 0)
    {
        return size << 1;
    }
    return size;
}

