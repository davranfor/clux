/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_MATCH_H
#define CLIB_MATCH_H

const char *test_mask(const char *, const char *);

int test_is_date(const char *);
int test_is_time(const char *);
int test_is_date_time(const char *);
int test_is_date_time_local(const char *);
int test_is_hostname(const char *);
int test_is_email(const char *);
int test_is_ipv4(const char *);
int test_is_ipv6(const char *);
int test_is_uuid(const char *);
int test_is_url(const char *);
int test_is_identifier(const char *);

int test_match(const char *, const char *);

#endif

