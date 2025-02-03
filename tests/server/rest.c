/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "rest.h"

static buffer_t buffer;

static void free_buffer(void)
{
    free(buffer.text);
}

static map_t *map;

static void destroy_map(void)
{
    map_destroy(map, json_free);
}

__attribute__((constructor))
static void load(void)
{
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    atexit(destroy_map);
    atexit(free_buffer);
}

static char *stringify(const json_t *node)
{
    if (node != NULL)
    {
        buffer_reset(&buffer);
        return json_buffer_encode(&buffer, node, 0);
    }
    return NULL;
}

char *rest_get(const char *uri)
{
    return stringify(map_search(map, uri));
}

char *rest_post(const char *uri, const char *content)
{
    json_t *object = json_parse(content, NULL);

    if (object == NULL)
    {
        return NULL;
    }
    json_object_delete(object, "id");

    static size_t id = 1;
    json_t *child = json_new_number(id);
    char key[64];

    snprintf(key, sizeof key, "%s/%zu", uri, id);
    if (!json_object_push(object, 0, "id", child) ||
        (map_insert(map, key, object) != object))
    {
        json_delete(object);
        json_delete(child);
        return NULL;
    }
    id += 1;
    return stringify(object);
}

char *rest_put(const char *uri, const char *content)
{
    json_t *old = map_search(map, uri);

    if (old == NULL)
    {
        return NULL;
    }

    json_t *new = json_parse(content, NULL);

    if (!json_equal(json_find(new, "id"), json_find(old, "id")) ||
        !map_update(map, uri, new))
    {
        json_delete(new);
        return NULL;
    }
    json_delete(old);
    return stringify(new);
}

char *rest_patch(const char *uri, const char *content)
{
    json_t *target = map_search(map, uri);

    if (target == NULL)
    {
        return NULL;
    }

    json_t *source = json_parse(content, NULL);

    if (source == NULL)
    {
        return NULL;
    }

    size_t id = json_size_t(json_find(target, "id"));
    int patch = json_patch(source, target);

    if (patch == -1)
    {
        target = NULL;
    }
    else if (json_size_t(json_find(target, "id")) != id)
    {
        json_unpatch(source, target, patch);
        target = NULL;
    }
    json_delete(source);
    return stringify(target);
}

char *rest_delete(const char *uri)
{
    json_t *node = map_delete(map, uri);
    char *str = stringify(node);

    json_delete(node);
    return str;
}

size_t rest_length(void)
{
    return buffer.length;
}

