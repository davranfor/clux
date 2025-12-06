/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_DATE_H
#define CLIB_DATE_H

int days_in_month(int, int);
int day_of_week(int, int, int);
int ISO_day_of_week(int, int, int);
int day_of_year(int, int, int);
int week_of_month(int, int, int);
int week_of_year(int, int, int);
void date_now(int *, int *, int *);
void date_add(int *, int *, int *, int);
int date_diff(int, int, int, int, int, int);
int is_date(int, int, int);
int is_leap(int);
long timestamp_ms(void);

#endif

