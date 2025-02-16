/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_POINTER_H
#define JSON_POINTER_H

#include "clib_buffer.h"
#include "json_header.h"

typedef struct
{
    const json_t *root;
    unsigned *path;
    unsigned size;
} json_pointer_t;

json_t *json_pointer(const json_t *, const char *);
json_t *json_extract(const json_pointer_t *);
char *json_write_pointer(buffer_t *, const json_pointer_t *);
char *json_write_pointer_max(buffer_t *, const json_pointer_t *, size_t);

#endif

