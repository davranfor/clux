/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include "headers.h"
#include "static.h"

static buffer_t bad_request;
static buffer_t not_found;
static map_t *map;

static void free_buffer(void *buffer)
{
    if (buffer != NULL)
    {
        free(((buffer_t *)buffer)->text);
        free(buffer);
    }
}

static void static_unload(void)
{
    free(bad_request.text);
    free(not_found.text);
    map_destroy(map, free_buffer);
}

void static_load(void)
{
    atexit(static_unload);
    if (!buffer_write(&bad_request, http_bad_request) ||
        !buffer_write(&not_found, http_not_found))
    {
        perror("buffer_write");
        exit(EXIT_FAILURE);
    }
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
}

static const char *path_type(const char *path)
{
    const char *extension = strrchr(path, '.');

    if ((extension == NULL) || (extension == path))
    {
        return NULL;
    }

    const char *extensions[][2] =
    {
        {".html", "text/html"}, 
        {".css", "text/css"},
        {".js", "application/javascript"}
    };
    size_t n = sizeof extensions / sizeof extensions[0];

    for (size_t i = 0; i < n; i++)
    {
        if (!strcmp(extension, extensions[i][0]))
        {
            return extensions[i][1];
        }
    }
    return NULL;
}

int static_push(const char *path)
{
    if (path == NULL)
    {
        return 0;
    }

    const char *type = path_type(path);

    if (type == NULL)
    {
        return 1;
    }

    printf("Loading '%s'\n", path);

    char *file = file_read(path);

    if (file == NULL)
    {
        perror("file_read");
        return 0;
    }

    buffer_t *buffer = calloc(1, sizeof *buffer);

    if (buffer == NULL)
    {
        perror("calloc");
        free(file);
        return 0;
    }

    size_t length = strlen(file);

    buffer->text = file;
    buffer->size = length + 1;
    buffer->length = length;

    char headers[128];

    snprintf(headers, sizeof headers, http_ok, type, length); 
    if (!buffer_insert(buffer, 0, headers, strlen(headers)))
    {
        perror("buffer_insert");
        free_buffer(buffer);
        return 0;
    }

    const char *filename = strrchr(path, '/');

    filename = filename ? filename + 1 : path;

    const buffer_t *temp = map_insert(map, filename, buffer);

    if (temp != buffer)
    {
        if (temp == NULL)
        {
            perror("map_insert");
        }
        else
        {
            fprintf(stderr, "'%s' already mapped\n", path);
        }
        free_buffer(buffer);
        return 0;
    }
    return 1;
}

static const buffer_t *search(const char *resource)
{
    buffer_t *buffer = map_search(map, resource);

    if (buffer != NULL)
    {
        return buffer;
    }
    return &not_found;
}

const buffer_t *static_buffer(const char *resource)
{
    if (resource == NULL)
    {
        return &bad_request;
    }
    if (resource[0] == '\0')
    {
        return search("index.html");
    }
    return search(resource);
}

const buffer_t *static_error(void)
{
    return &bad_request;
}

