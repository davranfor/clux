/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_STREAM_H
#define CLIB_STREAM_H

char *file_read(const char *);
char *file_quote(const char *, const char *prefix, const char *suffix);
int file_write(const char *, const char *);

#endif

