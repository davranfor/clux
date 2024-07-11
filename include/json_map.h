/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_MAP_H
#define JSON_MAP_H

#include "json_header.h"

typedef struct json_map json_map;
typedef int (*json_map_walk_callback)(json *, size_t, void *);

json_map *json_map_create(size_t);
json *json_map_update(json_map *, const char *, json *);
json *json_map_insert(json_map *, const char *, json *);
json *json_map_upsert(json_map *, const char *, json *);
json *json_map_delete(json_map *, const char *);
json *json_map_search(const json_map *, const char *);
json *json_map_walk(const json_map *, json_map_walk_callback, void *);
size_t json_map_size(const json_map *);
void json_map_destroy(json_map *, void (*)(json *));

#endif

