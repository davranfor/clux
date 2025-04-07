/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <clux/clib.h>
#include <clux/json.h>
#include <clux/json_private.h>
#include "headers.h"
#include "schema.h"
#include "static.h"
#include "writer.h"

static buffer_t buffer;
static buffer_t no_content;
static json_t *metadata;
static sqlite3 *db;

static void load(void)
{
    if (!buffer_write(&no_content, http_no_content))
    {
        perror("buffer_write");
        exit(EXIT_FAILURE);
    }

    json_error_t error = {0};

    printf("Loading 'clux.json'\n");
    if (!(metadata = json_parse_file("clux.json", &error)))
    {
        json_print_error(&error);
        exit(EXIT_FAILURE);
    }
    printf("Loading 'clux.db'\n");
    if (sqlite3_open("clux.db", &db))
    {
        fprintf(stderr, "Database error: %s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
}

static void unload(void)
{
    free(buffer.text);
    free(no_content.text);
    json_free(metadata);
    sqlite3_close(db);
}

void writer_load(void)
{
    atexit(unload);
    load();
}

/*
static char *encode(const json_t *node)
{
    return json_buffer_encode(&buffer, node, 0);
}

static char *api_get(const json_t *request)
{
    const char *path = json_string(json_find(request, "path"));

    return encode(map_search(map, path));
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

*/

static int validate(const json_t *rules, json_t *request)
{
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
        buffer_write(&buffer, "Error parsing content");
        json_print_error(&error);
        json_free(node);
        return 0;
    }

    json_t entry =
    {
        .type = JSON_OBJECT,
        .child = (json_t *[]){request},
        .size = 1
    };

    if (!schema_validate(rules, &entry, &buffer))
    {
        json_free(node);
        return 0;
    }
    return 1;
}

const buffer_t *writer_handle(json_t *request)
{
    buffer_reset(&buffer);
    json_print(request);

    const json_t *schema = schema_get(json_text(json_head(json_find(request, "path"))));

    if (schema == NULL)
    {
        return static_bad_request();
    }
    if (!validate(schema, request))
    {
        char headers[128];

        snprintf(headers, sizeof headers, http_bad_request, "text/plain", buffer.length);
        buffer_insert(&buffer, 0, headers, strlen(headers));
        return &buffer;
    }
    buffer_reset(&buffer);

    const char *content = NULL;
/*
    switch (select_method(request->key))
    {
        case GET:
            content = api_get(request);
            break;
        case POST:
            content = api_post(request);
            break;
        case PUT:
            content = api_put(request);
            break;
        case PATCH:
            content = api_patch(request);
            break;
        case DELETE:
            content = api_delete(request);
            break;
        default:
            break;
    }
*/
    json_free(json_find(request, "content"));
    if (content != NULL)
    {
        char headers[128];

        snprintf(headers, sizeof headers, http_ok, "application/json", buffer.length);
        buffer_insert(&buffer, 0, headers, strlen(headers));
        return &buffer;
    }
    return &no_content;
}

