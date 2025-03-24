/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h> // For debug
#include <string.h>
#include <clux/clib_unicode.h>
#include <clux/json_private.h>
#include "static.h"
#include "writer.h"
#include "parser.h"

#define CHILD_SIZE 8

struct request { char *method, *path, *parameters, *content; };

static char *parse_content(char *str)
{
    str = strstr(str, "\r\n\r\n");
    str[2] = '\0';
    return str + 4;
}

static int parse_headers(struct request *request, char *str)
{
    const char *array[] = {"GET /", "POST /", "PUT /", "PATCH /", "DELETE /"};
    size_t methods = sizeof array / sizeof array[0];

    for (size_t method = 0; method < methods; method++)
    {
        size_t length = strlen(array[method]);

        if (!strncmp(str, array[method], length))
        {
            char *path = str + length - 1;

            path[-1] = '\0';

            char *end = strchr(path, ' ');

            if (end == NULL)
            {
                return 0;
            }
            *end = '\0';
            request->method = str;
            request->path = path;
            request->parameters = strchr(path, '?');
            if (request->parameters != NULL)
            {
                *request->parameters = '\0';
                request->parameters++;
            }
            return 1;
        }
    }
    return 0;
}

static int parse_path(json_t *parent, json_t *child, char *str)
{
    unsigned size = 0;

    while ((size < CHILD_SIZE) && (*str == '/'))
    {
        *str++ = '\0';
        child[size].string = str;
        child[size].type = JSON_STRING;
        parent->child[size] = &child[size];
        parent->size = ++size;
        str += strcspn(str, "/");
    }
    return *str == '\0';
}

static int decode_parameters(json_t *parameters, char *str)
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
            if (parameters[size].key != NULL)
            {
                return 0;
            }
            parameters[size].key = key;
            parameters[size].string = ptr + 1;
            parameters[size].type = JSON_STRING;
            *ptr++ = '\0';
            str++;
        }
        else if (str[0] == '&')
        {
            if (parameters[size++].key == NULL)
            {
                return 0;
            }
            key = ptr + 1;
            *ptr++ = '\0';
            str++;
        }
        else if (str[0] == '\0')
        {
            if (parameters[size++].key == NULL)
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

static int parse_parameters(json_t *parent, json_t *child, char *str)
{
    if (str == NULL)
    {
        parent->child = NULL;
        parent->type = JSON_NULL;
        return 1;
    }
    if (!decode_parameters(child, str))
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

const buffer_t *parser_handle(char *message)
{
    puts(message);

    struct request request = {.content = parse_content(message)};

    if (!strstr(message, "Content-Type: application/json\r\n"))
    {
        return static_handle(message);
    }
    if (!parse_headers(&request, message))
    {
        return static_error();
    }

    json_t path[CHILD_SIZE] = {0}, parameters[CHILD_SIZE] = {0};
    json_t node[] =
    {
        {
            .key = "method",
            .string = request.method,
            .type = JSON_STRING
        },
        {
            .key = "path",
            .child = (json_t *[CHILD_SIZE]){0},
            .type = JSON_ARRAY
        },
        {
            .key = "parameters",
            .child = (json_t *[CHILD_SIZE]){0},
            .type = JSON_OBJECT
        },
        {
            .key = "content",
            .string = request.content,
            .type = JSON_STRING
        }
    };

    if (!parse_path(&node[1], path, request.path) ||
        !parse_parameters(&node[2], parameters, request.parameters))
    {
        return static_error();
    }
    return writer_handle(&(json_t)
    {
        .child = (json_t *[]){&node[0], &node[1], &node[2], &node[3]},
        .size = sizeof node / sizeof *node,
        .type = JSON_OBJECT
    });
}

