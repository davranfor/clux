/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "headers.h"
#include "writer.h"

static buffer_t buffer;
static buffer_t method_not_allowed;
static buffer_t no_content;

static void free_buffers(void)
{
    free(buffer.text);
    free(method_not_allowed.text);
    free(no_content.text);
}

static map_t *map;

static void destroy_map(void)
{
    map_destroy(map, json_free);
}

__attribute__((constructor))
static void load(void)
{
    if (!buffer_write(&method_not_allowed, http_method_not_allowed) ||
        !buffer_write(&no_content, http_no_content))
    {
        perror("buffer_write");
        exit(EXIT_FAILURE);
    }
    atexit(free_buffers);
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    atexit(destroy_map);
}

static char *stringify(const json_t *node)
{
    return json_buffer_encode(&buffer, node, 0);
}

static char *rest_get(const char *uri)
{
    return stringify(map_search(map, uri));
}

static char *rest_post(const char *uri, const char *content)
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

static char *rest_put(const char *uri, const char *content)
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

static char *rest_patch(const char *uri, const char *content)
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

static char *rest_delete(const char *uri)
{
    json_t *node = map_delete(map, uri);
    char *str = stringify(node);

    json_delete(node);
    return str;
}

enum method {GET, POST, PUT, PATCH, DELETE, METHODS};

static enum method select_method(const char *message)
{
    static const char *name[] = {"GET /", "POST /", "PUT /", "PATCH /", "DELETE /"};
    enum method method;

    for (method = 0; method < METHODS; method++)
    {
        if (!strncmp(message, name[method], strlen(name[method])))
        {
            break;
        }
    }
    return method;
}

const buffer_t *writer_handle(const char *message, const char *uri, const char *content)
{
    buffer_reset(&buffer);
    switch (select_method(message))
    {
        case GET:
            content = rest_get(uri);
            break;
        case POST:
            content = rest_post(uri, content);
            break;
        case PUT:
            content = rest_put(uri, content);
            break;
        case PATCH:
            content = rest_patch(uri, content);
            break;
        case DELETE:
            content = rest_delete(uri);
            break;
        default:
            return &method_not_allowed;
    }
    if (content != NULL)
    {
        char headers[96];

        snprintf(headers, sizeof headers, http_json_ok, buffer.length);
        buffer_insert(&buffer, 0, headers, strlen(headers));
        return buffer.error ? NULL : &buffer;
    }
    return &no_content;
}

