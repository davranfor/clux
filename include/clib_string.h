/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_STRING_H
#define CLIB_STRING_H

#include <stddef.h>
#include <stdarg.h>

char *string_clone(const char *);
char *string_format(const char *, ...) __attribute__ ((format (printf, 1, 2)));
char *string_vprint(const char *, va_list);
char *string_search(const char *, size_t, const char *, size_t);
size_t string_length(const char *);
size_t string_count(const char *, char);

#endif

