/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_HEADER_H
#define JSON_HEADER_H

#include <stddef.h>

#define JSON_HEAD 0
#define JSON_TAIL ((unsigned)-1)

#define JSON_NOT_FOUND -1u

/* Cast 'const json_t *' to 'json_t *' without warning */
#define json_cast(node)                                                     \
    _Pragma("GCC diagnostic push")                                          \
    _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                       \
    _Generic((node), const json_t *: ((json_t *)(node)), default: (node))   \
    _Pragma("GCC diagnostic pop")

enum json_type
{
    JSON_UNDEFINED,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
};

typedef struct json json_t;

#endif

