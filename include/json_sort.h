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
int JSON_SORT_BY_KEY_ASC(const json *, const json *);
int JSON_SORT_BY_KEY_DESC(const json *, const json *);
int JSON_SORT_BY_VALUE_ASC(const json *, const json *);
int JSON_SORT_BY_VALUE_DESC(const json *, const json *);
// End predefined sort callbacks
void json_sort(json *, json_sort_callback);
void json_reverse(json *);

#endif

