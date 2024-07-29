/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_LIST_H
#define JSON_LIST_H

#include "json_header.h"

typedef struct json_list json_list;
typedef int (*json_list_callback)(json_list *, json *, size_t, void *);

json_list *json_list_create(size_t);
json *json_list_push(json_list *, json *);
json *json_list_pop(json_list *);
json *json_list_at(const json_list *, size_t);
json **json_list_data(const json_list *);
size_t json_list_size(const json_list *);
int json_list_filter(json_list *, json *, json_list_callback, void *);
// Predefined sort callbacks
int JSON_LIST_SORT_BY_KEY_ASC(const void *, const void *);
int JSON_LIST_SORT_BY_KEY_DESC(const void *, const void *);
int JSON_LIST_SORT_BY_VALUE_ASC(const void *, const void *);
int JSON_LIST_SORT_BY_VALUE_DESC(const void *, const void *);
// End predefined sort callbacks
void json_list_sort(json_list *, int (*)(const void *, const void *));
void json_list_reverse(json_list *);
void json_list_free(json_list *);

#endif

