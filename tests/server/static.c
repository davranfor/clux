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

static void free_buffers(void)
{
    free(buffer.text);
    free(not_found.text);
}

__attribute__((constructor))
static void load(void)
{
    if (!buffer_write(&not_found, http_not_found))
    {
        perror("buffer_write");
        exit(EXIT_FAILURE);
    }
    atexit(free_buffers);
}

char *static_load(const char *index_html)
{
    size_t length = strlen(index_html);
    char headers[96];

    snprintf(headers, sizeof headers, http_html_ok, length);
    buffer_write(&buffer, headers);
    return buffer_append(&buffer, index_html, length);
}

buffer_t *static_handle(const char *headers) 
{
    return !strncmp(headers, "GET / ", 6) ? &buffer : &not_found;
}

