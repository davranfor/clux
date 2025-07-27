/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <time.h>
#include "clib_date.h"

static int leap_count(int year, int month)
{
    int years = year;

    if (month <= 2)
    {
        years--;
    }
    return (years / 4) - (years / 100) + (years / 400);
}

int days_in_month(int year, int month)
{
    static const int days[2][12] =
    {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
    };

    return days[is_leap(year)][month - 1];
}

/**
 * Tomohiko Sakamoto's Algorithm
 * Sunday = 0 ... Saturday = 6
 */
int day_of_week(int year, int month, int day)
{
    static const int offset[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

    year -= month < 3;
    return (year + year / 4 - year / 100 + year / 400 + offset[month - 1] + day) % 7;
}

/**
 * ISO 8601 date and time standard
 * Monday = 1 ... Sunday = 7
 */
int ISO_day_of_week(int year, int month, int day)
{
    static const int offset[] = {6, 2, 1, 4, 6, 2, 4, 0, 3, 5, 1, 3};

    year -= month < 3;
    return (year + year / 4 - year / 100 + year / 400 + offset[month - 1] + day) % 7 + 1;
}

int day_of_year(int year, int month, int day)
{
    static const int days[2][12] =
    {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };

    return days[is_leap(year)][month - 1] + day;
}

int week_of_month(int year, int month, int day)
{
    return (day - ISO_day_of_week(year, month, day) + 10) / 7;
}

int week_of_year(int year, int month, int day)
{
    return (day_of_year(year, month, day) - ISO_day_of_week(year, month, day) + 10) / 7;
}

void date_now(int *year, int *month, int *day)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    *day = tm.tm_mday;
    *month = tm.tm_mon + 1;
    *year = tm.tm_year + 1900;
}

void date_add(int *year, int *month, int *day, int days)
{
    struct tm tm = {0};

    tm.tm_mday = *day + days;
    tm.tm_mon = *month - 1;
    tm.tm_year = *year - 1900;

    mktime(&tm);

    *day = tm.tm_mday;
    *month = tm.tm_mon + 1;
    *year = tm.tm_year + 1900;
}

int date_diff(int year1, int month1, int day1, int year2, int month2, int day2)
{
    static const int days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    long int days1 = (year1 * 365) + day1;
    long int days2 = (year2 * 365) + day2;

    days1 += days[month1 - 1] + leap_count(year1, month1);
    days2 += days[month2 - 1] + leap_count(year2, month2);
    return (int)(days2 - days1);
}

void time_now(int *hour, int *minutes, int *seconds)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    *hour = tm.tm_hour;
    *minutes = tm.tm_min;
    *seconds = tm.tm_sec;
}

void date_time_now(
    int *year, int *month, int *day,
    int *hour, int *minutes, int *seconds)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    *day = tm.tm_mday;
    *month = tm.tm_mon + 1;
    *year = tm.tm_year + 1900;
    *hour = tm.tm_hour;
    *minutes = tm.tm_min;
    *seconds = tm.tm_sec;
}

int is_date(int year, int month, int day)
{
    if ((month < 1) || (month > 12))
    {
        return 0;
    }
    if ((day < 1) || (day > days_in_month(year, month)))
    {
        return 0;
    }
    return 1;
}

int is_time(int hour, int minutes, int seconds)
{
    return (hour >= 0) && (hour < 24)
        && (minutes >= 0) && (minutes < 60)
        && (seconds >= 0) && (seconds < 60);
}

int is_date_time(
    int year, int month, int day,
    int hour, int minutes, int seconds)
{
    return is_date(year, month, day) && is_time(hour, minutes, seconds);
}

int is_leap(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

