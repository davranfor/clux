/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <string.h>
#include <clux/clib_unicode.h>
#include <clux/json_private.h>
#include "loader.h"
#include "static.h"
#include "writer.h"
#include "parser.h"

#define CHILD_SIZE 8

enum { BAD_REQUEST, UNAUTHORIZED, RELOAD, STATIC, OK };

typedef struct { char *method, *path, *query, *content; } request_t;

static int parse_static(auth_t *auth, request_t *request, char *str)
{
    if (!strncmp(str, "GET /", 5))
    {
        char *path = str + 5;
        char *end = strchr(path, ' ');

        if (end == NULL)
        {
            return BAD_REQUEST;
        }
        *end = '\0';
        if (end == path)
        {
            auth->role = 1;
        }
        printf("role: %d\nkey:  %s\n", auth->role, auth->key);
        request->path = path;
        return STATIC;
    }
    if (!strncmp(str, "POST /reload ", 13))
    {
        loader_reload();
        return RELOAD;
    }
    return BAD_REQUEST;
}

static int parse_headers(auth_t *auth, request_t *request, char *str)
{
    char *content = strstr(str, "\r\n\r\n") + 4;

    if (*content != '\0')
    {
        request->content = content;
    }
    content[-2] = '\0';

    const char *array[] =
    {
        "GET /api/", "POST /api/", "PUT /api/", "PATCH /api/", "DELETE /api/"
    };
    size_t methods = sizeof array / sizeof array[0];

    for (size_t method = 0; method < methods; method++)
    {
        size_t length = strlen(array[method]);

        if (!strncmp(str, array[method], length))
        {
            if (auth->role == 0)
            {
                return UNAUTHORIZED;
            }

            char *path = str + length - 1;

            path[-5] = '\0';

            char *end = strchr(path, ' ');

            if (end == NULL)
            {
                return BAD_REQUEST;
            }
            *end = '\0';
            request->method = str;
            request->path = path;
            request->query = strchr(path, '?');
            if (request->query != NULL)
            {
                *request->query++ = '\0';
            }
            return OK;
        }
    }
    return parse_static(auth, request, str);
}

static int parse_path(json_t *parent, json_t *child, char *str)
{
    unsigned size = 0;

    while ((size < CHILD_SIZE) && (*str == '/'))
    {
        if (size == 0)
        {
            child[size].string = str++;
        }
        else
        {
            child[size].string = ++str;
            str[-1] = '\0';
        }
        child[size].type = JSON_STRING;
        parent->child[size] = &child[size];
        parent->size = ++size;
        str += strcspn(str, "/");
    }
    return *str == '\0';
}

static int decode_query(json_t *query, char *str)
{
    char *key = str, *ptr = str;
    unsigned size = 0;

    while (size < CHILD_SIZE)
    {
        if ((str[0] == '%') && is_xdigit(str[1]) && is_xdigit(str[2]))
        {
            int hi = hex_to_dec(str[1]);
            int lo = hex_to_dec(str[2]);

            *ptr++ = (char)((hi << 4) | lo);
            str += 3;
        }
        else if (str[0] == '+')
        {
            *ptr++ = ' ';
            str++;
        }
        else if (str[0] == '=')
        {
            if (query[size].key != NULL)
            {
                return 0;
            }
            query[size].key = key;
            query[size].string = ptr + 1;
            query[size].type = JSON_STRING;
            *ptr++ = '\0';
            str++;
        }
        else if (str[0] == '&')
        {
            if (query[size++].key == NULL)
            {
                return 0;
            }
            key = ptr + 1;
            *ptr++ = '\0';
            str++;
        }
        else if (str[0] == '\0')
        {
            if (query[size++].key == NULL)
            {
                return 0;
            }
            *ptr = '\0';
            return 1;
        }
        else
        {
            *ptr++ = *str++;
        }
    }
    return 0;
}

static int parse_query(json_t *parent, json_t *child, char *str)
{
    if (str == NULL)
    {
        parent->child = NULL;
        parent->type = JSON_NULL;
        return 1;
    }
    if (!decode_query(child, str))
    {
        return 0;
    }
    for (unsigned i = 0; i < CHILD_SIZE; i++)
    {
        if (child[i].type == JSON_STRING)
        {
            parent->child[i] = &child[i];
            parent->size++;
        }
    }
    return 1;
}

const buffer_t *parser_handle(auth_t *auth, char *message)
{
    (void)auth;
    printf("----------------------------------------------------------------------\n%s\n", message);

    request_t request = {0};

    switch (parse_headers(auth, &request, message))
    {
        case BAD_REQUEST:
            return static_bad_request();
        case UNAUTHORIZED:
            return static_unauthorized();
        case RELOAD:
            return static_no_content();
        case STATIC:
            return static_get(request.path);
    }

    json_t path[CHILD_SIZE] = {0}, query[CHILD_SIZE] = {0};
    json_t node[] =
    {
        {
            .key = "path",
            .child = (json_t *[CHILD_SIZE]){0},
            .type = JSON_ARRAY
        },
        {
            .key = "query",
            .child = (json_t *[CHILD_SIZE]){0},
            .type = JSON_OBJECT
        },
        {
            .key = "user",
            .number = auth->user,
            .type = JSON_INTEGER
        },
        {
            .key = "role",
            .number = auth->role,
            .type = JSON_INTEGER
        },
        {
            .key = "content",
            .string = request.content,
            .type = request.content ? JSON_STRING : JSON_NULL
        }
    };

    if (!parse_path(&node[0], path, request.path) ||
        !parse_query(&node[1], query, request.query))
    {
        return static_bad_request();
    }
    return writer_handle(&(json_t)
    {
        .key = request.method,
        .child = (json_t *[]){&node[0], &node[1], &node[2], &node[3], &node[4]},
        .size = sizeof node / sizeof *node,
        .type = JSON_OBJECT
    });
}

