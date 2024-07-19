/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_HEADER_H
#define JSON_HEADER_H

/**
 * This header is included in all json_?.h headers
 * stddef.h is included here - is required for size_t
 * ... so we don't need to include it in every header
 */

#include <stddef.h>

#define JSON_MAX_DECIMALS 17

enum json_type
{
    JSON_UNDEFINED,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_BOOLEAN,
    JSON_NULL,
};

typedef struct json json;

#endif

