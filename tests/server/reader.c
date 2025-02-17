/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "reader.h"

#define HEADERS_MAX_LENGTH 4096

int reader_handle(char *message, size_t length)
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

