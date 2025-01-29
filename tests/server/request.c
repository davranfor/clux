/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "request.h"

#define HEADERS_MAX_LENGTH 4096

static const char *http_html_ok =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_json_ok =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_no_content =
    "HTTP/1.1 204 No Content\r\n\r\n";
static const char *http_not_found =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n\r\n"
    "404 Not Found";
static const char *http_method_not_allowed =
    "HTTP/1.1 405 Method Not Allowed\r\n"
    "Allow: GET, POST, PUT, DELETE, PATCH\r\n"
    "Content-Length: 0\r\n\r\n";

enum method {GET, POST, PUT, PATCH, DELETE, METHODS, UNKNOWN = METHODS, NONE};
static const char *method_name[] =
{
    "GET /",
    "POST /",
    "PUT /",
    "PATCH /",
    "DELETE /",
};

static map_t *map;

static void unload_map(void)
{
    map_destroy(map, json_free);
}

__attribute__((constructor))
static void load_map(void)
{
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    atexit(unload_map);
}

ssize_t request_handle(char *text, size_t length)
{
    char *delimiter = strstr(text, "\r\n\r\n");

    if (delimiter == NULL)
    {
        return length > HEADERS_MAX_LENGTH ? 0 : -1;
    }
    delimiter[0] = '\0';

    const char *content_length_mark = strstr(text, "Content-Length:");

    delimiter[0] = '\r';
    if (content_length_mark == NULL)
    {
        return delimiter[4] == '\0';
    }

    size_t headers_length = (size_t)(delimiter - text) + 4;
    size_t content_length = strtoul(content_length_mark + 15, NULL, 10);
    size_t request_length = headers_length + content_length;

    return length < request_length ? -1 : length == request_length;
}

// cppcheck-suppress constParameterPointer
static const char *parse_uri(char *headers)
{
    const char *uri = strchr(headers, '/');

    if (uri == NULL)
    {
        return NULL;
    }

    char *end = strchr(uri, ' ');

    if (end == NULL)
    {
        return NULL;
    }
    end[0] = '\0';
    return uri;
}

static enum method parse_method(const char *headers)
{
    enum method method;

    for (method = 0; method < METHODS; method++)
    {
        if (!strncmp(headers, method_name[method], strlen(method_name[method])))
        {
            break;
        }
    }
    return method;
}

static char *api_get(const char *uri)
{
    return json_stringify(map_search(map, uri));
}

static char *api_post(const char *uri, const char *content)
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
    return json_stringify(object);
}

static char *api_put(const char *uri, const char *content)
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
    return json_stringify(new);
}

static char *api_patch(const char *uri, const char *content)
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
    return json_stringify(target);
}

static char *api_delete(const char *uri)
{
    json_t *node = map_delete(map, uri);
    char *str = json_stringify(node);

    json_delete(node);
    return str;
}

static char *do_request(char *headers, const char *content, enum method *method)
{
    if (strstr(headers, "Content-Type: application/json\r\n") == NULL)
    {
        return !strncmp(headers, "GET / ", 6) ? file_read("www/index.html") : NULL;
    }

    const char *uri = parse_uri(headers);

    if (uri == NULL)
    {
        return NULL;
    }
    switch ((*method = parse_method(headers)))
    {
        case GET:
            return api_get(uri);
        case POST:
            return api_post(uri, content);
        case PUT:
            return api_put(uri, content);
        case PATCH:
            return api_patch(uri, content);
        case DELETE:
            return api_delete(uri);
        default:
            return NULL;
    }
}

static const char *http_ok(enum method method)
{
    switch (method)
    {
        case NONE:
            return http_html_ok;
        default:
            return http_json_ok;
    }
}

static const char *http_ko(enum method method)
{
    switch (method)
    {
        case NONE:
            return http_not_found;
        case UNKNOWN:
            return http_method_not_allowed;
        default:
            return http_no_content;
    }
}

void request_reply(pool_t *pool, char *buffer, size_t size)
{
    enum method method = NONE;
    char *content = strstr(pool->text, "\r\n\r\n") + 4;

    content[-4] = '\0';
    content = do_request(pool->text, content, &method);
    if (content == NULL)
    {
        const char *headers = http_ko(method);
        size_t headers_length = strlen(headers);

        if (headers_length < size)
        {
            memcpy(buffer, headers, headers_length + 1);
            pool_bind(pool, buffer, headers_length);
        }
        else
        {
            pool_reset(pool);
            if (!pool_put(pool, headers, headers_length))
            {
                perror("pool_put");
                pool_reset(pool);
            }
        }
    }
    else
    {
        size_t content_length = strlen(content);
        const char *headers_fmt = http_ok(method);
        char headers[256];

        snprintf(headers, sizeof headers, headers_fmt, content_length);

        size_t headers_length = strlen(headers);

        if (headers_length + content_length < size)
        {
            memcpy(buffer, headers, headers_length);
            memcpy(buffer + headers_length, content, content_length + 1);
            pool_bind(pool, buffer, headers_length + content_length);
        }
        else
        {
            pool_reset(pool);
            if (!pool_put(pool, headers, headers_length) ||
                !pool_put(pool, content, content_length))
            {
                perror("pool_put");
                pool_reset(pool);
            }
        }
        free(content);
    }
}

