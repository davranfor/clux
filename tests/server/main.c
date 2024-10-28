/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "config.h"
#include "server.h"

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

static int request_ready(const char *str, size_t size)
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

static char *request_put(const char *uri, const char *content)
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

static char *request_patch(const char *uri, const char *content)
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

static char *request_delete(const char *uri)
{
    json_t *node = json_map_delete(map, uri);
    char *str = json_encode(node);

    json_delete(node);
    return str;
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

static char *request_content(char *header, const char *content,
    enum method *method)
{
    if (strstr(header, content_type_json) == NULL)
    {
        return strncmp(header, "GET / ", 6) == 0
            ? file_read("www/index.html")
            : NULL;
    }

    const char *uri = request_uri(header);

    if (uri == NULL)
    {
        return NULL;
    }
    switch ((*method = request_method(header)))
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

static void request_reply(struct poolfd *pool, char *buffer, size_t size)
{
    enum method method = NONE;
    char *content = strstr(pool->data, header_end);

    if (content[HEADER_END_LENGTH] != '\0')
    {
        content[0] = '\0';
        content += HEADER_END_LENGTH;
    }
    else
    {
        content = NULL;
    }
    content = request_content(pool->data, content, &method);
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

static uint16_t port_number(const char *str)
{
    char *end;
    unsigned long result = strtoul(str, &end, 10);

    if ((result > 65535) || (end[strspn(end, " \f\n\r\t\v")] != '\0'))
    {
        return 0;
    }
    return (uint16_t)result;
}

static int populate_schemas(void)
{
    struct dirent *dir;
    DIR *schemas;

    schemas = opendir("schemas");
    if (schemas == NULL)
    {
        fprintf(stderr, "'schemas' dir must exist\n");
        return 0;
    }

    const char ext[] = ".schema.json"; 
    int fail = 0;

    while ((dir = readdir(schemas)))
    {
        const char *d_name_ext = strstr(dir->d_name, ext);

        if ((d_name_ext == NULL) || (strlen(d_name_ext) != sizeof(ext) -1))
        {
            continue;
        }

        char path[255];

        snprintf(path, sizeof path, "schemas/%s", dir->d_name);

        json_error_t error;
        json_t *node = json_parse_file(path, &error);

        if (node == NULL)
        {
            fprintf(stderr, "%s\n", path);
            json_print_error(&error);
            fail = 1;
            break;
        }
        if (!json_schema_map(node))
        {
            fprintf(stderr, "Error mapping '%s' -> '$id' must exist\n", path);
            json_delete(node);
            fail = 1;
            break;
        }
    }
    closedir(schemas);
    return !fail;
}

int main(int argc, char *argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-h") == 0))
    {
        printf("Usage: %s [port]\n", argv[0]);
        return 0;
    }

    uint16_t port = argc > 1 ? port_number(argv[1]) : SERVER_PORT;

    if (port == 0)
    {
        fprintf(stderr, "Invalid port\n");
        exit(EXIT_FAILURE);
    }
    setlocale(LC_CTYPE, "");
    atexit(map_destroy);
    if ((map = json_map_create(0)) == NULL)
    {
        perror("json_map_create");
        exit(EXIT_FAILURE);
    }
    if (!populate_schemas())
    {
        exit(EXIT_FAILURE);
    }
    server_init(port, request_ready, request_reply);
    return 0;
}

