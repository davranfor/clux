/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_STREAM_H
#define CLIB_STREAM_H

int file_exists(const char *);
char *file_read(const char *);
char *file_read_callback(const char *, char *(*)(const void *, size_t), const void *);
int file_write(const char *, const char *);
int file_write_bytes(const char *, const char *, size_t);
int file_delete(const char *);

#endif

