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

static char *rest_get(const char *resource)
{
    return encode(map_search(map, resource));
}

static char *rest_post(const char *resource, const char *content)
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

static char *rest_put(const char *resource, const char *content)
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

static char *rest_patch(const char *resource, const char *content)
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

static char *rest_delete(const char *resource)
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
    if (path->size > 1)
    {
        path->child[1]->string[- 1] = '/';
        path->size = 1;
    }
    return path->child[0]->string;
}

const buffer_t *writer_handle(json_t *request, const char *content)
{
    buffer_reset(&buffer);
    json_print(request);

    const char *resource = join_path(request->child[1]);

    switch (select_method(request->child[0]->string))
    {
        case GET:
            content = rest_get(resource);
            break;
        case POST:
            content = rest_post(resource, content);
            break;
        case PUT:
            content = rest_put(resource, content);
            break;
        case PATCH:
            content = rest_patch(resource, content);
            break;
        case DELETE:
            content = rest_delete(resource);
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

