/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include "http.h"
#include "rest.h"
#include "router.h"

enum method {GET, POST, PUT, PATCH, DELETE, METHODS, UNKNOWN = METHODS, NONE};

ssize_t router_parse(char *text, size_t length)
{
    char *delimiter = strstr(text, "\r\n\r\n");

    if (delimiter == NULL)
    {
        return length > HTTP_HEADERS_MAX_LENGTH ? 0 : -1;
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
    static const char *name[] = {"GET /", "POST /", "PUT /", "PATCH /", "DELETE /"};
    enum method method;

    for (method = 0; method < METHODS; method++)
    {
        if (!strncmp(headers, name[method], strlen(name[method])))
        {
            break;
        }
    }
    return method;
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
            return rest_get(uri);
        case POST:
            return rest_post(uri, content);
        case PUT:
            return rest_put(uri, content);
        case PATCH:
            return rest_patch(uri, content);
        case DELETE:
            return rest_delete(uri);
        default:
            return NULL;
    }
}

static const char *response_ok(enum method method)
{
    switch (method)
    {
        case NONE:
            return http_html_ok;
        default:
            return http_json_ok;
    }
}

static const char *response_ko(enum method method)
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

void router_reply(pool_t *pool, char *buffer, size_t size)
{
    enum method method = NONE;
    char *content = strstr(pool->text, "\r\n\r\n") + 4;

    content[-4] = '\0';
    content = do_request(pool->text, content, &method);
    if (content != NULL)
    {
        const char *headers_fmt = response_ok(method);
        size_t content_length;

        if (method == NONE)
        {
            content_length = strlen(content);
        }
        else
        {
            content_length = rest_length();
        }

        char headers[256];

        snprintf(headers, sizeof headers, headers_fmt, content_length);

        size_t headers_length = strlen(headers);

        if (headers_length + content_length <= size)
        {
            memcpy(buffer, headers, headers_length);
            memcpy(buffer + headers_length, content, content_length);
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
        if (method == NONE)
        {
            free(content);
        }
    }
    else
    {
        const char *headers = response_ko(method);
        size_t headers_length = strlen(headers);

        if (headers_length <= size)
        {
            memcpy(buffer, headers, headers_length);
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
}

