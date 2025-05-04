/*!
 *  \brief     C library for unixes
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
    if ((path == NULL) || (path->type != JSON_STRING))
    {
        return 0;
    }
    node->string = path->string;
    return 1;
}

static int on_notify(const json_event_t *event)
{
    json_t *node = json_pointer(event->node, "/path/0");
    const json_t *path = json_find(event->rule, "path");

    if (!set_path(node, path))
    {
        return STOP; 
    }
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

int schema_validate(json_t *request, buffer_t *buffer)
{
    json_print(request);
    buffer_reset(buffer);

    const char *path = json_string(json_pointer(request, "/path/0")); 
    const json_t *rules = map_search(map, path);

    if (rules == NULL)
    {
        buffer_format(buffer, "Collection '%s' not found", path);
        return 0;
    }

    unsigned content_id = json_index(request, "content");
    json_t *content = request->child[content_id];
    const char *text = "null";

    if (content->type == JSON_STRING)
    {
        text = content->string;
    }

    json_error_t error;
    json_t *node = json_parse(text, &error);

    if (node && json_set_key(node, "content"))
    {
        request->child[content_id] = node;
    }
    else
    {
        buffer_write(buffer, "Error parsing content");
        json_print_error(&error);
        json_free(node);
        return 0;
    }

    json_t entry = {.type = JSON_OBJECT, .child = (json_t *[]){request}, .size = 1};
    context_t context = {.buffer = buffer, .path = path, .result = 0};
    int result = HTTP_OK;

    if (!json_validate(rules, &entry, map, on_validate, &context))
    {
        request->child[content_id] = content;
        result = context.result;
        json_free(node);
    }
    return result;
}

