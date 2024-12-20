/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_POINTER_H
#define JSON_POINTER_H

#include "json_header.h"

json_t *json_pointer(const json_t *, const char *);
size_t json_pointer_put_key(char *, size_t, const char *);
size_t json_pointer_put_index(char *, size_t, size_t);

#endif

