/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_SCHEMA_H
#define JSON_SCHEMA_H

#include "clib_hashmap.h"
#include "clib_buffer.h"
#include "json_header.h"

/* Unknown events mode */
enum json_warning_mode {JSON_WARNINGS_OFF, JSON_WARNINGS_ON, JSON_WARNING_AS_ERROR};
/* Event types */
enum {JSON_WARNING, JSON_FAILURE, JSON_ABORTED};
/* Event responses */
enum {JSON_ABORT, JSON_CONTINUE};

typedef struct
{
    int type;
    const char *path;
    const json_t *node, *rule;
} json_event_t;

typedef int (*json_validate_callback)(const json_event_t *, void *);

void json_set_warning_mode(enum json_warning_mode);
enum json_warning_mode json_get_warning_mode(void);
char *json_write_event(const json_event_t *, buffer_t *, size_t);
int json_validate(const json_t *, const json_t *, const map_t *,
    json_validate_callback, void *);

#endif

