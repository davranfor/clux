/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_SCHEMA_H
#define JSON_SCHEMA_H

#include "clib_hashmap.h"
#include "json_header.h"

typedef struct { int type; const json_t *path, *node, *rule; } json_schema_event_t;
typedef int (*json_validate_callback)(const json_schema_event_t *, void *);

/* Events */
enum {JSON_SCHEMA_WARNING, JSON_SCHEMA_FAILURE, JSON_SCHEMA_ABORTED};
/* Response */
enum {JSON_SCHEMA_STOP, JSON_SCHEMA_CONTINUE};

json_t *json_schema(const char *);
void json_schema_set_map(map_t *);
map_t *json_schema_get_map(void);

int json_validate(const json_t *, const json_t *, json_validate_callback, void *);

#endif

