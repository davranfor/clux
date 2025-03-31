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
#include "schema.h"
#include "static.h"
#include "writer.h"

// Temporary (move this)
#include "loader.h"

static buffer_t buffer;
static buffer_t method_not_allowed;
static buffer_t no_content;
static map_t *map;

static void load(void)
{
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

static void unload(void)
{
    free(buffer.text);
    free(method_not_allowed.text);
    free(no_content.text);
    map_destroy(map, json_free);
}

void writer_load(void)
{
    atexit(unload);
    load();
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
    unsigned size = 1;

    while (size < path->size)
    {
        path->child[size++]->string[-1] = '/';
    }
    if (path->size > 1)
    {
        path->size = 1;
    }
    return path->child[0]->string;
}

static int validate(const json_t *schema, json_t *request)
{
    json_t *content = json_find(request, "content");
    char *content_text = content->string;
    int has_content = *content_text;

    if (has_content)
    {
        json_error_t error;
        json_t *node = json_parse(content_text, &error);

        if ((node == NULL) || (node->type != JSON_OBJECT))
        {
            json_print_error(&error);
            buffer_write(&buffer, "Error parsing file");
            return 0;
        }
        else
        {
            json_print(node);
        }
        content->child = node->child;
        content->type = JSON_OBJECT;
        content->size = node->size;

        int rc = schema_validate(schema, request, &buffer);

        content->string = content_text;
        content->type = JSON_STRING;
        content->size = 0;
        json_free(node);
        return rc;
    }
    return schema_validate(schema, request, &buffer);
}

const buffer_t *writer_handle(json_t *request)
{
    buffer_reset(&buffer);
    json_print(request);

    const json_t *schema = schema_get(json_text(json_head(json_find(request, "path"))));

    if (schema == NULL)
    {
        return static_not_found();
    }
    if (!validate(schema, request))
    {
        char headers[128];

        snprintf(headers, sizeof headers, http_bad_request, "text/plain", buffer.length);
        buffer_insert(&buffer, 0, headers, strlen(headers));
        return buffer.error ? NULL : &buffer;
    }
    buffer_reset(&buffer);

    const char *path = join_path(json_find(request, "path"));
    const char *content = json_text(json_find(request, "content"));

    switch (select_method(json_text(json_find(request, "method"))))
    {
        case GET:
            // Temporary
            if (!strcmp(path, "/reload"))
            {
                loader_reload();
                return &no_content;
            }
            // End temporary
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

        snprintf(headers, sizeof headers, http_ok, "application/json", buffer.length);
        buffer_insert(&buffer, 0, headers, strlen(headers));
        return buffer.error ? NULL : &buffer;
    }
    return &no_content;
}

