/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_MAP_H
#define JSON_MAP_H

#include "json_header.h"

typedef struct json_map json_map_t;
typedef int (*json_map_walk_callback)(json_t *, size_t, void *);

json_map_t *json_map_create(size_t);
json_t *json_map_update(json_map_t *, const char *, json_t *);
json_t *json_map_insert(json_map_t *, const char *, json_t *);
json_t *json_map_upsert(json_map_t *, const char *, json_t *);
json_t *json_map_delete(json_map_t *, const char *);
json_t *json_map_search(const json_map_t *, const char *);
json_t *json_map_walk(const json_map_t *, json_map_walk_callback, void *);
size_t json_map_size(const json_map_t *);
void json_map_destroy(json_map_t *, void (*)(json_t *));

#endif

