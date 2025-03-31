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

enum {CONTINUE, STOP};

static int on_failure(const json_event_t *event, buffer_t *buffer)
{
    if (buffer->length > BUFFER_LIMIT)
    {
        buffer_write(buffer, "...\n");
        return STOP;
    }
    if (!json_write_event(buffer, event, ENCODE_MAX))
    {
        return STOP;
    }
    return CONTINUE;
}

static int on_warning(const json_t *rule)
{
    fprintf(stderr, "Warning: Unknow rule '%s'\n", json_name(rule));
    return CONTINUE;
}

static int on_notify(const json_t *rule)
{
    fprintf(stderr, "Notify: "); 
    json_write_line(rule, stderr);
    return CONTINUE;
}

static int on_error(const json_t *rule)
{
    fprintf(stderr, "Aborted: Malformed schema\n"); 
    json_write_line(rule, stderr);
    return STOP;
}

static int on_validate(const json_event_t *event, void *buffer)
{
    switch (event->type)
    {
        case JSON_FAILURE:
            return on_failure(event, buffer);
        case JSON_WARNING:
            return on_warning(event->rule);
        case JSON_NOTIFY:
            return on_notify(event->rule);
        case JSON_ERROR:
            return on_error(event->rule);
    }
    return STOP;
}

int schema_validate(const json_t *rules, const json_t *entry, buffer_t *buffer)
{
    int rc = 1;

    if (!json_validate(rules, entry, map, on_validate, buffer))
    {
        fprintf(stderr, "Doesn't validate against schema\n");
        rc = 0;
    }
    return rc;
}

