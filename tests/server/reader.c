/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include "parser.h"
#include "reader.h"

#define HEADERS_MAX_LENGTH 4096

int reader_status(char *message, size_t length)
{
    char *delimiter = strstr(message, "\r\n\r\n");

    if (delimiter == NULL)
    {
        return length > HEADERS_MAX_LENGTH ? 0 : -1;
    }
    delimiter[0] = '\0';

    const char *content_length_mark = strstr(message, "Content-Length:");

    delimiter[0] = '\r';
    if (content_length_mark == NULL)
    {
        return delimiter[4] == '\0';
    }

    size_t headers_length = (size_t)(delimiter - message) + 4;
    size_t content_length = strtoul(content_length_mark + 15, NULL, 10);
    size_t request_length = headers_length + content_length;

    return length < request_length ? -1 : length == request_length;
}

void reader_handle(pool_t *pool, char *buffer, size_t size)
{
    const buffer_t *message = parser_handle(pool->text);

    if (message != NULL)
    {
        if (message->length <= size)
        {
            memcpy(buffer, message->text, message->length);
            pool_bind(pool, buffer, message->length);
        }
        else
        {
            pool_reset(pool);
            if (!pool_put(pool, message->text, message->length))
            {
                perror("pool_put");
            }
        }
    }
    else
    {
        pool_reset(pool);
    }
}

