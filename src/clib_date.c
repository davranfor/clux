/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "clib_date.h"

static int year_is_leap(int year)
{
    return (((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0);
}

static int month_days(int month, int year)
{
    static const int days[2][12] =
    {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
    };
    int leap = year_is_leap(year);

    return days[leap][month - 1];
}

int is_date(int year, int month, int day)
{
    if ((month < 1) || (month > 12))
    {
        return 0;
    }
    if ((day < 1) || (day > month_days(month, year)))
    {
        return 0;
    }
    return 1;
}

