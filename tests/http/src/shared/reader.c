/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <clux/clib_string.h>
#include "reader.h"

#define HEADERS_MAX_LENGTH 4096

int reader_handle(const char *message, size_t length)
{
    char *delimiter = string_search(message, length, "\r\n\r\n", 4);

    if (delimiter == NULL)
    {
        return length > HEADERS_MAX_LENGTH ? 0 : -1;
    }

    size_t headers_length = (size_t)(delimiter - message);
    const char *length_mark = string_search(
        message, headers_length, "Content-Length:", 15
    );

    if (length_mark == NULL)
    {
        return delimiter[4] == '\0';
    }

    size_t content_length = strtoul(length_mark + 15, NULL, 10);
    size_t request_length = headers_length + content_length + 4;

    return length < request_length ? -1 : length == request_length;
}

