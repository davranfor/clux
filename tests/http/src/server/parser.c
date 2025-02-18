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

#define MAX_PARAMS 8

enum {METHOD, RESOURCE, PARAMS, REQUEST_SIZE};

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

static char *parse_resource(char *message)
{
    char *resource = parse_method(message);

    if (resource == NULL)
    {
        return NULL;
    }

    char *end = strchr(resource, ' ');

    if (end == NULL)
    {
        return NULL;
    }
    end[0] = '\0';
    return resource;
}

static int decode_params(json_t *params, char *str)
{
    char *key = str, *ptr = str;
    short size = 0;

    while (size < MAX_PARAMS)
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

static int parse_params(json_t *request, json_t *child[])
{
    char *params = strchr(request[RESOURCE].string, '?');

    if (params == NULL)
    {
        return 1;
    }
    params[0] = '\0';

    json_t *array = request + PARAMS + 1;

    if (!decode_params(array, params + 1))
    {
        return 0;
    }

    json_t *object = request + PARAMS;

    for (short i = 0; i < MAX_PARAMS; i++)
    {
        if (array[i].type == JSON_STRING)
        {
            child[i] = &array[i];
            object->size++;
        }
    }
    object->child = child;
    return 1;
}

const buffer_t *parser_handle(char *message)
{
    puts(message);

    const char *content = parse_content(message);

    if (!strstr(message, "Content-Type: application/json\r\n"))
    {
        return static_handle(message);
    }

    char *resource = parse_resource(message);

    if (resource == NULL)
    {
        return static_error(); 
    }

    json_t child[REQUEST_SIZE + MAX_PARAMS] =
    {
        {
            .key = "method",
            .string = message,
            .type = JSON_STRING
        },
        {
            .key = "resource",
            .string = resource,
            .type = JSON_STRING
        },
        {
            .key = "params",
            .type = JSON_OBJECT
        }
    };
    json_t *params[MAX_PARAMS] = {0};

    if (!parse_params(child, params))
    {
        return static_error();
    }

    json_t request =
    {
        .child = (json_t *[]){&child[METHOD], &child[RESOURCE], &child[PARAMS]},
        .size = REQUEST_SIZE,
        .type = JSON_OBJECT
    };

    return writer_handle(&request, content);
}

