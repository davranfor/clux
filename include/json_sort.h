/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_SORT_H
#define JSON_SORT_H

#include "json_header.h"

typedef int (*json_sort_callback)(const json *, const json *);

// Predefined sort callbacks
int json_compare_key_asc(const json *, const json *);
int json_compare_key_desc(const json *, const json *);
int json_compare_value_asc(const json *, const json *);
int json_compare_value_desc(const json *, const json *);
// End predefined sort callbacks
void json_sort(json *, json_sort_callback);
void json_reverse(json *);

#endif

