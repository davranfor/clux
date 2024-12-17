/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_BUFFER_H
#define CLIB_BUFFER_H

typedef struct { char *text; size_t length, size; } buffer_t;

char *buffer_resize(buffer_t *, size_t);
char *buffer_append(buffer_t *, const char *, size_t);
char *buffer_write(buffer_t *, const char *);

#endif

