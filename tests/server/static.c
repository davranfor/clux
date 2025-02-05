/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers.h"
#include "static.h"

static buffer_t buffer;
static buffer_t not_found;

static void static_unload(void)
{
    free(buffer.text);
    free(not_found.text);
}

void static_load(char *index_html)
{
    atexit(static_unload);

    size_t length = strlen(index_html);

    buffer.text = index_html;
    buffer.size = length + 1;
    buffer.length = length;

    char headers[96];

    snprintf(headers, sizeof headers, http_html_ok, length);
    buffer_insert(&buffer, 0, headers, strlen(headers));
    buffer_write(&not_found, http_not_found);
    if (buffer.error || not_found.error)
    {
        perror("buffer_write");
        exit(EXIT_FAILURE);
    }
}

buffer_t *static_handle(const char *headers) 
{
    return !strncmp(headers, "GET / ", 6) ? &buffer : &not_found;
}

