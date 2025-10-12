/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_HEADER_H
#define JSON_HEADER_H

#define JSON_HEAD 0
#define JSON_TAIL ((unsigned)-1)

#define JSON_NOT_FOUND -1u

enum json_type
{
    JSON_UNDEFINED = 0,
    JSON_OBJECT = 1,
    JSON_ARRAY = 2,
    JSON_STRING = 4,
    JSON_INTEGER = 8,
    JSON_REAL = 16,
    JSON_TRUE = 32,
    JSON_FALSE = 64,
    JSON_NULL = 128,
};

enum
{
    JSON_NUMBER = JSON_INTEGER | JSON_REAL,
    JSON_BOOLEAN = JSON_TRUE | JSON_FALSE,
    JSON_ITERABLE = JSON_OBJECT | JSON_ARRAY,
    JSON_SCALAR = JSON_STRING | JSON_NUMBER | JSON_BOOLEAN | JSON_NULL,
};

typedef struct json json_t;

static inline json_t *json_cast(const json_t *node)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    return (json_t *)node;
#pragma GCC diagnostic pop
}

#endif

