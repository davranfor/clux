/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "json_header.h"

typedef int (*json_sort_callback)(const void *, const void *);

int json_compare(const json_t *, const json_t *);
json_t *json_search(const json_t *, const json_t *, json_sort_callback);
void json_sort(json_t *, json_sort_callback);
void json_reverse(json_t *);

#endif

