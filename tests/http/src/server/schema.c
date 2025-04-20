/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <clux/clib.h>
#include <clux/json.h>
#include <clux/json_private.h>
#include "headers.h"
#include "schema.h"

static map_t *map;

static void load(void)
{
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
}

static void unload(void)
{
    map_destroy(map, json_free);
}

void schema_load(void)
{
    atexit(unload);
    load();
}

void schema_reload(void)
{
    unload();
    load();
}

int schema_add(const char *path)
{
    if (path == NULL)
    {
        return 0;
    }

    const char *extension = strrchr(path, '.');

    if (!extension || strcmp(extension, ".json"))
    {
        return 1;
    }
    printf("Loading '%s'\n", path);

    json_error_t error;
    json_t *node = json_parse_file(path, &error);

    if (node == NULL)
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
        return 0;
    }

    const char *id = json_string(json_find(node, "$id"));

    if (id == NULL)
    {
        fprintf(stderr, "Error mapping '%s' -> '$id' must exist\n", path);
        json_delete(node);
        return 0;
    }

    const json_t *temp = map_insert(map, id, node);

    if (temp != node)
    {
        fprintf(stderr, "'%s' %s\n", id, temp ? "already mapped" : strerror(errno));
        json_delete(node);
        return 0;
    }
    return 1;
}

const json_t *schema_get(const char *id)
{
    return map_search(map, id);
}

#define BUFFER_LIMIT 4096   // Don't write to buffer after this limit
#define ENCODE_MAX 128      // Max length of an event line

typedef struct
{
    buffer_t *buffer;
    const char *path;
    int result;
} context_t;

enum {CONTINUE, STOP};

static int set_path(json_t *node, const json_t *path)
{
    if ((node == NULL) || (node->type != JSON_STRING) ||
        (path == NULL) || (path->type != JSON_STRING))
    {
        return 0;
    }
    node->string = path->string;
    return 1;
}

static int on_notify(const json_event_t *event)
{
    const json_t *path = json_find(event->rule, "path");

    if (!set_path(json_head(json_find(event->node, "path")), path))
    {
        return STOP; 
    }
    json_print(event->node);
    return CONTINUE;
}

static int on_warning(const json_t *rule, const context_t *context)
{
    fprintf(stderr, "Warning: Unknow rule '%s' at schema '%s'\n",
        json_name(rule), context->path);
    return CONTINUE;
}

static int on_failure(const json_event_t *event, context_t *context)
{
    if (context->buffer->length > BUFFER_LIMIT)
    {
        buffer_write(context->buffer, "...\n");
        return STOP;
    }
    if (!json_write_event(context->buffer, event, ENCODE_MAX))
    {
        return STOP;
    }
    return CONTINUE;
}

static int on_error(const json_t *rule, context_t *context)
{
    fprintf(stderr, "Aborted: Malformed schema '%s'\n", context->path); 
    json_write_line(rule, stderr);
    buffer_format(context->buffer, "Malformed schema '%s'", context->path);
    context->result = HTTP_SERVER_ERROR;
    return STOP;
}

static int on_validate(const json_event_t *event, void *context)
{
    switch (event->type)
    {
        case JSON_NOTIFY:
            return on_notify(event);
        case JSON_WARNING:
            return on_warning(event->rule, context);
        case JSON_FAILURE:
            return on_failure(event, context);
        case JSON_ERROR:
            return on_error(event->rule, context);
    }
    return STOP;
}

int schema_validate(const json_t *rules, const json_t *entry, buffer_t *buffer,
    const char *path)
{
    context_t context = {buffer, path, 0};

    if (!json_validate(rules, entry, map, on_validate, &context))
    {
        return context.result;
    }
    return HTTP_OK;
}

