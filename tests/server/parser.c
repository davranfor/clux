/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include "static.h"
#include "writer.h"
#include "parser.h"

static const char *parse_content(char *message)
{
    char *content = strstr(message, "\r\n\r\n");

    content[0] = '\0';
    return content + 4;
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

buffer_t *parser_handle(char *message)
{
    const char *content = parse_content(message);

    if (strstr(message, "Content-Type: application/json\r\n") == NULL)
    {
        return static_handle(message);
    }

    const char *uri = parse_uri(message);

    if (uri != NULL)
    {
        return writer_handle(message, uri, content);
    }
    return NULL;
}

