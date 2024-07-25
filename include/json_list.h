/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_LIST_H
#define JSON_LIST_H

#include "json_header.h"
#include "json_sort.h"

typedef struct json_list json_list;

json_list *json_list_create(size_t);
int json_list_add(json_list *, const json *);
const json *json_list_at(const json_list *, size_t);
const json **json_list_data(const json_list *);
size_t json_list_size(const json_list *);
void json_list_sort(json_list *, json_sort_callback);
void json_list_destroy(json_list *);

#endif

