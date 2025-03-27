/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include <clux/json.h>
#include <clux/json_private.h>
#include "headers.h"
#include "writer.h"

static buffer_t buffer;
static buffer_t method_not_allowed;
static buffer_t no_content;
static map_t *map;

static void writer_unload(void)
{
    free(buffer.text);
    free(method_not_allowed.text);
    free(no_content.text);
    map_destroy(map, json_free);
}

void writer_load(void)
{
    atexit(writer_unload);
    if (!buffer_write(&method_not_allowed, http_method_not_allowed) ||
        !buffer_write(&no_content, http_no_content))
    {
        perror("buffer_write");
        exit(EXIT_FAILURE);
    }
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
}

static char *encode(const json_t *node)
{
    return json_buffer_encode(&buffer, node, 0);
}

static char *api_get(const char *resource)
{
    return encode(map_search(map, resource));
}

static char *api_post(const char *resource, const char *content)
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

    snprintf(key, sizeof key, "%s/%zu", resource, id);
    if (!json_object_push(object, 0, "id", child) ||
        (map_insert(map, key, object) != object))
    {
        json_delete(object);
        json_delete(child);
        return NULL;
    }
    id++;
    return encode(object);
}

static char *api_put(const char *resource, const char *content)
{
    json_t *old = map_search(map, resource);

    if (old == NULL)
    {
        return NULL;
    }

    json_t *new = json_parse(content, NULL);

    if (!json_equal(json_find(new, "id"), json_find(old, "id")) ||
        !map_update(map, resource, new))
    {
        json_delete(new);
        return NULL;
    }
    json_delete(old);
    return encode(new);
}

static char *api_patch(const char *resource, const char *content)
{
    json_t *target = map_search(map, resource);

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
    char *str = NULL;

    if ((patch == -1) || (json_size_t(json_find(target, "id")) != id))
    {
        json_unpatch(source, target, patch);
    }
    else
    {
        str = encode(target);
    }
    json_delete(source);
    return str;
}

static char *api_delete(const char *resource)
{
    json_t *node = map_delete(map, resource);
    char *str = encode(node);

    json_delete(node);
    return str;
}

enum method {GET, POST, PUT, PATCH, DELETE, METHODS};

static enum method select_method(const char *message)
{
    const char *name[] = {"GET", "POST", "PUT", "PATCH", "DELETE"};
    enum method method;

    for (method = 0; method < METHODS; method++)
    {
        if (!strcmp(message, name[method]))
        {
            break;
        }
    }
    return method;
}

static const char *join_path(json_t *path)
{
    unsigned size = 0;

    while (size < path->size)
    {
        path->child[size++]->string[-1] = '/';
    }
    if (path->size > 1)
    {
        path->size = 1;
    }
    return path->child[0]->string - 1;
}

const buffer_t *writer_handle(json_t *request)
{
    buffer_reset(&buffer);
    json_print(request);

    const char *path = join_path(json_find(request, "path"));
    const char *content = json_text(json_find(request, "content"));

    switch (select_method(json_text(json_find(request, "method"))))
    {
        case GET:
            content = api_get(path);
            break;
        case POST:
            content = api_post(path, content);
            break;
        case PUT:
            content = api_put(path, content);
            break;
        case PATCH:
            content = api_patch(path, content);
            break;
        case DELETE:
            content = api_delete(path);
            break;
        default:
            return &method_not_allowed;
    }
    if (content != NULL)
    {
        char headers[128];

        snprintf(headers, sizeof headers, http_json_ok, buffer.length);
        buffer_insert(&buffer, 0, headers, strlen(headers));
        return buffer.error ? NULL : &buffer;
    }
    return &no_content;
}

