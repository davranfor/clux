/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_QUERY_H
#define JSON_QUERY_H

#include "json_header.h"

int json_is(const json *, const char *);
int json_is_unique(const json *);

#endif

