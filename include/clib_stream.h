/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_STREAM_H
#define CLIB_STREAM_H

enum { FILE_WRITE, FILE_APPEND };

int file_exists(const char *);
char *file_read(const char *);
char *file_read_callback(const char *, char *(*)(void *, size_t), void *);
int file_write(const char *, const char *);
int file_write_bytes(const char *, const char *, size_t);
int file_append(const char *, const char *);
int file_append_bytes(const char *, const char *, size_t);
int file_delete(const char *);

#endif

