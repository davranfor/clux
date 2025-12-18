/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_STREAM_H
#define CLIB_STREAM_H

int file_exists(const char *);
char *file_read(const char *);
char *file_read_callback(const char *, char *(*)(void *, size_t), void *);
int file_write(const char *, const char *);

#endif

