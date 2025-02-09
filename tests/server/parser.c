/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include <clux/json_private.h>
#include "static.h"
#include "writer.h"
#include "parser.h"

#define MAX_PARAMS 8

static const char *parse_content(char *message)
{
    char *content = strstr(message, "\r\n\r\n");

    content[0] = '\0';
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

const buffer_t *parser_handle(char *message)
{
    const char *content = parse_content(message);

    if (!strstr(message, "Content-Type: application/json\r\n"))
    {
        return static_handle(message);
    }

    char *resource = parse_resource(message);

    if (resource == NULL)
    {
        return NULL;
    }

    json_t child[] =
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
            .child = (json_t *[MAX_PARAMS]){0},
            .size = 0,
            .type = JSON_OBJECT
        }
    };
    json_t request =
    {
        .child = (json_t *[]){&child[0], &child[1], &child[2]},
        .size = 3,
        .type = JSON_OBJECT
    };

    return writer_handle(&request, content);
}

