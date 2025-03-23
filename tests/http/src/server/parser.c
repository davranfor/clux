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

enum {METHOD, PATH, PARAMS, PARAMS_SIZE = 8};

static const char *parse_content(char *message)
{
    char *content = strstr(message, "\r\n\r\n");

    content[2] = '\0';
    return content + 4;
}

static char *parse_method(char *message)
{
    const char *array[] = {"GET /", "POST /", "PUT /", "PATCH /", "DELETE /"};
    size_t methods = sizeof array / sizeof array[0];

    for (size_t method = 0; method < methods; method++)
    {
        size_t length = strlen(array[method]);

        if (!strncmp(message, array[method], length))
        {
            char *resource = message + length - 1;

            resource[-1] = '\0';
            return resource;
        }
    }
    return NULL;
}

static int decode_params(json_t *params, char *str)
{
    char *key = str, *ptr = str;
    unsigned size = 0;

    while (size < PARAMS_SIZE)
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
            if (params[size].key != NULL)
            {
                return 0;
            }
            params[size].key = key;
            params[size].string = ptr + 1;
            params[size].type = JSON_STRING;
            *ptr++ = '\0';
            str++;
        }
        else if (str[0] == '&')
        {
            if (params[size++].key == NULL)
            {
                return 0;
            }
            key = ptr + 1;
            *ptr++ = '\0';
            str++;
        }
        else if (str[0] == '\0')
        {
            if (params[size++].key == NULL)
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

static int parse_params(json_t *params, json_t *array, char *path)
{
    if ((path = strchr(path, '?')))
    {
        *path = '\0';
    }
    else
    {
        return 1;
    }
    if (!decode_params(array, path + 1))
    {
        return 0;
    }
    for (unsigned i = 0; i < PARAMS_SIZE; i++)
    {
        if (array[i].type == JSON_STRING)
        {
            params->child[i] = &array[i];
            params->size++;
        }
    }
    return 1;
}

static int parse_path(json_t *request, json_t *params)
{
    char *path = parse_method(request[METHOD].string);

    if (path == NULL)
    {
        return 0;
    }

    char *end = strchr(path, ' ');

    if (end != NULL)
    {
        *end = '\0';
    }
    else
    {
        return 0;
    }

    char *next = strchr(path + 1, '/'); 

    if (next == NULL)
    {
        request[PATH].child[0]->string = path;
        request[PATH].child[0]->type = JSON_STRING;
        request[PATH].size = 1;
        return parse_params(&request[PARAMS], params, path);
    }
    else
    {
        *next++ = '\0';
        request[PATH].child[0]->string = path;
        request[PATH].child[0]->type = JSON_STRING;
        request[PATH].child[1]->string = next;
        request[PATH].child[1]->type = JSON_STRING;
        request[PATH].size = 2;
        return parse_params(&request[PARAMS], params, next);
    } 
}

const buffer_t *parser_handle(char *message)
{
    puts(message);

    const char *content = parse_content(message);

    if (!strstr(message, "Content-Type: application/json\r\n"))
    {
        return static_handle(message);
    }

    json_t path[2] = {0}, params[PARAMS_SIZE] = {0};
    json_t node[] =
    {
        {
            .key = "method",
            .string = message,
            .type = JSON_STRING
        },
        {
            .key = "path",
            .child = (json_t *[]){&path[0], &path[1]},
            .type = JSON_ARRAY
        },
        {
            .key = "params",
            .child = (json_t *[PARAMS_SIZE]){0},
            .type = JSON_OBJECT
        }
    };

    if (!parse_path(node, params))
    {
        return static_error();
    }

    json_t request =
    {
        .child = (json_t *[]){&node[METHOD], &node[PATH], &node[PARAMS]},
        .size = sizeof node / sizeof *node,
        .type = JSON_OBJECT
    };

    return writer_handle(&request, content);
}

