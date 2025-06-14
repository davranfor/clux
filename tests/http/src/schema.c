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
#include "header.h"
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

typedef struct { json_t *request; buffer_t *buffer; int result; } context_t;

enum {CONTINUE, STOP};

static const char *get_path(const context_t *context)
{
    return json_string(json_pointer(context->request, "/path/0"));
}

static int set_path(const json_event_t *event, context_t *context)
{
    const json_t *path = json_find(event->rule, "path");

    if ((path == NULL) || (path->type != JSON_STRING))
    {
        fprintf(stderr, "x-notify: 'path' was expected (%s)\n", get_path(context));
        buffer_format(context->buffer, "Malformed schema '%s'", get_path(context));
        context->result = HTTP_SERVER_ERROR;
        return 0;
    }

    json_t *node = json_find(context->request, "path");

    node->child[0]->string = path->string;

    const json_t *size = json_find(event->rule, "size");

    if (size == NULL)
    {
        return 1;
    }
    if ((size->type != JSON_INTEGER) ||
        (size->number < 1) || (size->number > node->size))
    {
        fprintf(stderr, "x-notify: 'size' wants an integer >0 <=%u\n", node->size);
        buffer_format(context->buffer, "Malformed schema '%s'", get_path(context));
        context->result = HTTP_SERVER_ERROR;
        return 0;
    }
    node->size = (unsigned)size->number;
    return 1;
}

static int test_role(const json_event_t *event, context_t *context)
{
    const json_t *role = json_find(event->rule, "role");

    if ((role == NULL) || (role->type != JSON_INTEGER) || (role->number < 0))
    {
        fprintf(stderr, "x-notify: A 'role' was expected (%s)\n", get_path(context));
        buffer_format(context->buffer, "Malformed schema '%s'", get_path(context));
        context->result = HTTP_SERVER_ERROR;
        return 0;
    }

    double number = json_pointer(context->request, "/cookie/role")->number;

    if (number < role->number)
    {
        buffer_write(context->buffer, "Forbidden");
        context->result = HTTP_FORBIDDEN;
        return 0;
    }
    return 1;
}

static int on_notify(const json_event_t *event, context_t *context)
{
    if (context->result != 0)
    {
        fprintf(stderr, "A result was already set at schema (%s)\n", get_path(context));
        buffer_format(context->buffer, "Malformed schema '%s'", get_path(context));
        context->result = HTTP_SERVER_ERROR;
        return STOP;
    }
    if (!set_path(event, context) || !test_role(event, context))
    {
        return STOP;
    }
    context->result = HTTP_OK;
    return CONTINUE;
}

static int on_warning(const json_event_t *event, const context_t *context)
{
    fprintf(stderr, "Warning: Unknow rule '%s' at schema '%s'\n",
        json_name(event->rule), get_path(context));
    return CONTINUE;
}

static int on_failure(const json_event_t *event, context_t *context)
{
    context->result = HTTP_BAD_REQUEST;
    if ((event->pointer->size < 2) ||
        (event->pointer->path[1] != json_index(context->request, "content")))
    {
    fprintf(stderr, "'%s' at schema '%s'\n",
        json_name(event->rule), get_path(context));
        return STOP;
    }
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

static int on_error(const json_event_t *event, context_t *context)
{
    fprintf(stderr, "Aborted: Malformed schema '%s'\n", get_path(context));
    json_write_line(event->rule, stderr);
    buffer_format(context->buffer, "Malformed schema '%s'", get_path(context));
    context->result = HTTP_SERVER_ERROR;
    return STOP;
}

static int on_validate(const json_event_t *event, void *context)
{
    switch (event->type)
    {
        case JSON_NOTIFY:
            return on_notify(event, context);
        case JSON_WARNING:
            return on_warning(event, context);
        case JSON_FAILURE:
            return on_failure(event, context);
        case JSON_ERROR:
            return on_error(event, context);
    }
    return STOP;
}

int schema_validate(json_t *request, buffer_t *buffer)
{
    json_write_line(request, stdout);
    buffer_reset(buffer);

    const char *path = json_string(json_pointer(request, "/path/0"));
    const json_t *rules = map_search(map, path);

    if (rules == NULL)
    {
        buffer_format(buffer, "Resource '%s' not found", path);
        return HTTP_NOT_FOUND;
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
        return HTTP_BAD_REQUEST;
    }

    json_t entry = {.type = JSON_OBJECT, .child = (json_t *[]){request}, .size = 1};
    context_t context = {.request = request, .buffer = buffer, .result = 0};
    int valid = json_validate(rules, &entry, map, on_validate, &context);

    if (context.result == 0)
    {
        fprintf(stderr, "A result was expected at schema '%s'\n", path);
        buffer_format(buffer, "Malformed schema '%s'", path);
        json_free(node);
        return HTTP_SERVER_ERROR;
    }

    int result = HTTP_OK;

    if (!valid || (context.result != HTTP_OK))
    {
        request->child[content_id] = content;
        result = context.result;
        json_free(node);
    }
    return result;
}

