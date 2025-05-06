/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <clux/clib.h>
#include "header.h"
#include "static.h"

static buffer_t no_content;
static buffer_t bad_request;
static buffer_t unauthorized;
static buffer_t forbidden;
static buffer_t not_found;
static map_t *map;

static int fill_buffer(buffer_t *buffer, const char *header, const char *text)
{
    size_t length = strlen(text);

    buffer_format(buffer, header, "text/plain", length);
    buffer_append(buffer, text, length);
    return !buffer->error;
}

static void free_buffer(void *buffer)
{
    if (buffer != NULL)
    {
        free(((buffer_t *)buffer)->text);
        free(buffer);
    }
}

static void load(void)
{
    if (!buffer_write(&no_content, http_no_content) ||
        !fill_buffer(&bad_request, http_bad_request, "Bad Request") ||
        !fill_buffer(&unauthorized, http_unauthorized, "Unauthorized") ||
        !fill_buffer(&forbidden, http_forbidden, "Forbidden") ||
        !fill_buffer(&not_found, http_not_found, "Not Found"))
    {
        perror("buffer");
        exit(EXIT_FAILURE);
    }
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
}

static void unload(void)
{
    buffer_clean(&no_content);
    buffer_clean(&bad_request);
    buffer_clean(&unauthorized);
    buffer_clean(&forbidden);
    buffer_clean(&not_found);
    map_destroy(map, free_buffer);
}

void static_load(void)
{
    atexit(unload);
    load();
}

void static_reload(void)
{
    unload();
    load();
}

static const char *path_type(const char *path)
{
    const char *type = strrchr(path, '.');

    if ((type == NULL) || (type == path))
    {
        return NULL;
    }

    const char *types[][2] =
    {
        {".html", "text/html"}, 
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"}
    };

    for (size_t i = 0; i < sizeof types / sizeof types[0]; i++)
    {
        if (!strcmp(type, types[i][0]))
        {
            return types[i][1];
        }
    }
    return NULL;
}

int static_add(const char *path)
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
    buffer->length = length;
    buffer->size = length + 1;

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
        fprintf(stderr, "'%s' %s\n", path, temp ? "already mapped" : strerror(errno));
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

const buffer_t *static_get(const char *resource)
{
    if (resource == NULL)
    {
        return &bad_request;
    }
    return search(*resource ? resource : "index.html");
}

const buffer_t *static_no_content(void)
{
    return &no_content;
}

const buffer_t *static_bad_request(void)
{
    return &bad_request;
}

const buffer_t *static_unauthorized(void)
{
    return &unauthorized;
}

const buffer_t *static_forbidden(void)
{
    return &forbidden;
}

const buffer_t *static_not_found(void)
{
    return &not_found;
}

