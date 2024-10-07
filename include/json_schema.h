/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_SCHEMA_H
#define JSON_SCHEMA_H

#include "json_header.h"

enum {JSON_SCHEMA_WARNING, JSON_SCHEMA_INVALID, JSON_SCHEMA_ERROR};

typedef int (*json_validate_callback)(const json_t *, const json_t *, int, void *);

int json_validate(const json_t *, const json_t *, json_validate_callback, void *);

#endif

