/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <math.h>
#include "clib_math.h"

int is_safe_number(double number)
{
    int exponent;
    double mantissa = frexp(number, &exponent);
 
    if ((exponent > -53) && (exponent < 53))
    {
        return 1;
    }
    if ((exponent == 53) || (exponent == -53))
    {
        return (mantissa > -1.0) && (mantissa < 1.0);
    }
    return 0; 
} 

