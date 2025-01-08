/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_BUFFER_H
#define CLIB_BUFFER_H

typedef struct { char *text; size_t length, size; int fail; } buffer_t;

char *buffer_resize(buffer_t *, size_t);
char *buffer_putchr(buffer_t *, char);
char *buffer_attach(buffer_t *, const char *, size_t);
char *buffer_insert(buffer_t *, size_t, const char *, size_t);
char *buffer_append(buffer_t *, const char *);
char *buffer_format(buffer_t *, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
void buffer_adjust(buffer_t *, size_t);

#endif

