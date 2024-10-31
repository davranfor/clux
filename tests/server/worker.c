/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "worker.h"

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
static const char *content_type_json =
    "Content-Type: application/json\r\n";
static const char *content_length_label =
    "Content-Length:";
static const char *header_end =
    "\r\n\r\n";

enum {HEADER_END_LENGTH = 4};

enum method {GET, POST, PUT, PATCH, DELETE, METHODS, UNKNOWN = METHODS, NONE};
static const char *method_name[] = {
    "GET /",
    "POST /",
    "PUT /",
    "PATCH /",
    "DELETE /",
};

static json_map_t *map;

static void map_destroy(void)
{
    json_map_destroy(map, json_free);
}

int worker_create_map(void)
{
    atexit(map_destroy);
    if (!(map = json_map_create(0)))
    {
        perror("json_map_create");
        return 0;
    }
    return 1;
}

int request_ready(const char *str, size_t size)
{
    const char *end = strstr(str, header_end);

    if (end == NULL)
    {
        return 0;
    }
    end += HEADER_END_LENGTH;

    const char *label = strstr(str, content_length_label);

    if (label == NULL)
    {
        return !*end;
    }
    if (label > end)
    {
        return 0;
    }

    size_t length = strtoul(label + strlen(content_length_label), NULL, 10);

    return size == (size_t)(end - str) + length;
}

// cppcheck-suppress constParameterPointer
static const char *parse_uri(char *header)
{
    const char *uri = strchr(header, '/');

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

static enum method parse_method(const char *header)
{
    enum method method;

    for (method = 0; method < METHODS; method++)
    {
        if (!strncmp(header, method_name[method], strlen(method_name[method])))
        {
            break;
        }
    }
    return method;
}

static char *api_get(const char *uri)
{
    return json_encode(json_map_search(map, uri));
}

static char *api_post(const char *uri, const char *content)
{
    json_t *node = json_parse(content, NULL);

    if (node == NULL)
    {
        return NULL;
    }
    json_delete(node, "id");

    static size_t id = 1;
    char key[64];

    snprintf(key, sizeof key, "%s/%zu", uri, id);
    if ((json_map_insert(map, key, node) != node) ||
        !json_push_front(node, "id", json_new_number(id)))
    {
        json_delete(node);
        node = NULL;
    }
    else
    {
        id += 1;
    }
    return json_encode(node);
}

static char *api_put(const char *uri, const char *content)
{
    json_t *old = json_map_search(map, uri);

    if (old == NULL)
    {
        return NULL;
    }

    json_t *new = json_parse(content, NULL);

    if (!json_equal(json_find(new, "id"), json_find(old, "id")) ||
        !json_map_update(map, uri, new))
    {
        json_delete(new);
        new = NULL;
    }
    else
    {
        json_delete(old);
    }
    return json_encode(new);
}

static char *api_patch(const char *uri, const char *content)
{
    json_t *target = json_map_search(map, uri);

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
    return json_encode(target);
}

static char *api_delete(const char *uri)
{
    json_t *node = json_map_delete(map, uri);
    char *str = json_encode(node);

    json_delete(node);
    return str;
}

static char *do_request(char *header, const char *content,
    enum method *method)
{
    if (strstr(header, content_type_json) == NULL)
    {
        return strncmp(header, "GET / ", 6) == 0
            ? file_read("www/index.html")
            : NULL;
    }

    const char *uri = parse_uri(header);

    if (uri == NULL)
    {
        return NULL;
    }
    switch ((*method = parse_method(header)))
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

static const char *request_header_ok(enum method method)
{
    switch (method)
    {
        case NONE:
            return http_html_ok;
        default:
            return http_json_ok;
    }
}

static const char *request_header_ko(enum method method)
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
    char *content = strstr(pool->data, header_end);

    content[0] = '\0';
    content += HEADER_END_LENGTH;
    content = do_request(pool->data, content, &method);
    if (content == NULL)
    {
        const char *header = request_header_ko(method);
        size_t header_length = strlen(header);

        if (header_length <= size)
        {
            memcpy(buffer, header, header_length);
            pool_set(pool, buffer, header_length);
        }
        else
        {
            pool_reset(pool);
            if (!pool_put(pool, header, header_length))
            {
                perror("pool_put");
                pool_reset(pool);
            }
        }
    }
    else
    {
        const char *header_fmt = request_header_ok(method);
        size_t content_length = strlen(content);
        char header[256];

        snprintf(header, sizeof header, header_fmt, content_length);

        size_t header_length = strlen(header);

        if (header_length + content_length <= size)
        {
            memcpy(buffer, header, header_length);
            memcpy(buffer + header_length, content, content_length);
            pool_set(pool, buffer, header_length + content_length);
        }
        else
        {
            pool_reset(pool);
            if (!pool_put(pool, header, header_length) ||
                !pool_put(pool, content, content_length))
            {
                perror("pool_put");
                pool_reset(pool);
            }
        }
        free(content);
    }
}

