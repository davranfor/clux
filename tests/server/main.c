/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <clux/json.h>
#include "config.h"
#include "server.h"
#include "utils.h"

static const char *content_type_json = "Content-Type: application/json\r\n";
static const char *content_length_label = "Content-Length:";
static const char *http_json_ok = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: %zu\r\n\r\n";
static const char *http_html_ok = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: text/html\r\n"
                                  "Content-Length: %zu\r\n\r\n";
static const char *http_ko = "HTTP/1.1 204 No Content\r\n\r\n";
static const char *delimiter = "\r\n\r\n";

enum {delimiter_length = 4};

enum method {GET, POST, PUT, PATCH, DELETE, METHODS};
static const char *method_name[] = {"GET", "POST", "PUT", "PATCH", "DELETE"};

static json_map *map;

static void map_destroy(void)
{
    json_map_destroy(map, json_free);
}

static int request_done(const char *str, size_t size)
{
    const char *end = strstr(str, delimiter);

    if (end == NULL)
    {
        return 0;
    }
    end += delimiter_length;


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

static enum method request_method(const char *header)
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

// cppcheck-suppress constParameterPointer
static const char *request_uri(char *header)
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

static char *request_get(const char *uri)
{
    return json_encode(json_map_search(map, uri));
}

static char *request_post(const char *uri, const char *content)
{
    json *node = json_parse(content, NULL);

    if (node == NULL)
    {
        return NULL;
    }
    json_delete(json_find(node, "id"));

    static size_t id = 1;
    char key[64];

    snprintf(key, sizeof key, "%s/%zu", uri, id);
    if ((json_map_insert(map, key, node) != node) ||
        !json_push_front(node, json_new_number("id", id)))
    {
        json_free(node);
        node = NULL;
    }
    else
    {
        id += 1;
    }
    return json_encode(node);
}

static char *request_put(const char *uri, const char *content)
{
    json *old = json_map_search(map, uri);

    if (old == NULL)
    {
        return NULL;
    }

    json *new = json_parse(content, NULL);

    if (!json_equal(json_find(new, "id"), json_find(old, "id")) ||
        !json_map_update(map, uri, new))
    {
        json_free(new);
        new = NULL;
    }
    else
    {
        json_free(old);
    }
    return json_encode(new);
}

static char *request_patch(const char *uri, const char *content)
{
    json *target = json_map_search(map, uri);

    if (target == NULL)
    {
        return NULL;
    }

    json *source = json_parse(content, NULL);

    if (source == NULL)
    {
        return NULL;
    }

    size_t id = json_size_t(json_find(target, "id"));
    int patch = json_patch(target, source);

    if (patch == -1)
    {
        target = NULL;
    }
    else if (json_size_t(json_find(target, "id")) != id)
    {
        json_unpatch(target, source, patch);
        target = NULL;
    }
    json_free(source);
    return json_encode(target);
}

static char *request_delete(const char *uri)
{
    json *node = json_map_delete(map, uri);
    char *str = json_encode(node);

    json_free(node);
    return str;
}

static char *request_result(char *header, const char *content)
{
    if (strstr(header, content_type_json) == NULL)
    {
        return strncmp(header, "GET / ", 6) == 0
            ? read_file("www/index.html")
            : NULL;
    }

    const char *uri = request_uri(header);

    if (uri == NULL)
    {
        return NULL;
    }
    switch (request_method(header))
    {
        case GET:
            return request_get(uri);
        case POST:
            return request_post(uri, content);
        case PUT:
            return request_put(uri, content);
        case PATCH:
            return request_patch(uri, content);
        case DELETE:
            return request_delete(uri);
        default:
            return NULL;
    }
}

static void request_handle(struct poolfd *pool, char *buffer, size_t size)
{
    char *content = strstr(pool->data, delimiter);

    if (content[delimiter_length] != '\0')
    {
        content[0] = '\0';
        content += delimiter_length;
    }
    else
    {
        content = NULL;
    }
    content = request_result(pool->data, content);
    if (content == NULL)
    {
        size_t header_length = strlen(http_ko);

        memcpy(buffer, http_ko, header_length);
        pool_set(pool, buffer, header_length);
    }
    else
    {
        const char *header_ok = content[0] == '{' ? http_json_ok : http_html_ok;
        size_t content_length = strlen(content);
        char header[256];

        snprintf(header, sizeof header, header_ok, content_length);

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

int main(int argc, char *argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-h") == 0))
    {
        printf("Usage: %s [port]\n", argv[0]);
        return 0;
    }

    uint16_t port = argc > 1 ? string_to_uint16(argv[1]) : SERVER_PORT;

    if (port == 0)
    {
        fprintf(stderr, "Invalid port\n");
        exit(EXIT_FAILURE);
    }
    atexit(map_destroy);
    if ((map = json_map_create(100)) == NULL)
    {
        perror("json_map_create");
        exit(EXIT_FAILURE);
    }
    server_init(port, request_done, request_handle);
    return 0;
}

