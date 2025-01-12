/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_BUFFER_H
#define CLIB_BUFFER_H

typedef struct { char *text; size_t length, size; int fail; } buffer_t;

char *buffer_resize(buffer_t *, size_t);
char *buffer_insert(buffer_t *, size_t, const char *, size_t);
char *buffer_append(buffer_t *, const char *, size_t);
char *buffer_write(buffer_t *, const char *);
char *buffer_print(buffer_t *, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
char *buffer_repeat(buffer_t *, char, size_t);
char *buffer_put(buffer_t *, char);
void buffer_set_length(buffer_t *, size_t);

#endif

