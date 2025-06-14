/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/evp.h>
#include <clux/clib.h>
#include "header.h"
#include "static.h"

#define ETAG_SIZE EVP_MAX_MD_SIZE

static buffer_t no_content;
static buffer_t not_modified;
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
    if (!buffer_format(&no_content, http_no_content, "") ||
        !buffer_format(&not_modified, http_not_modified, "") ||
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
    buffer_clean(&not_modified);
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
        {".json", "application/json"},
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

static int calculate_etag(const char *input, size_t length, char *output)
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    if (ctx == NULL)
    {
        return 0;
    }

    unsigned int size = 0;

    if (!EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) ||
        !EVP_DigestUpdate(ctx, input, length) ||
        !EVP_DigestFinal_ex(ctx, hash, &size))
    {
        EVP_MD_CTX_free(ctx);
        return 0;
    }
    EVP_MD_CTX_free(ctx);
    for (unsigned i = 0; i < size; i++)
    {
        snprintf(output + (i * 2), 3, "%02x", hash[i]);
    }
    output[size * 2] = '\0';
    return 1;
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

    char etag[ETAG_SIZE];

    if (!calculate_etag(buffer->text, buffer->length, etag))
    {
        perror("calculate_etag");
        free_buffer(buffer);
        return 0;
    }

    char headers[256];

    snprintf(headers, sizeof headers, http_no_cache, etag, type, length);
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

static const buffer_t *search(const char *resource, const char *headers)
{
    buffer_t *buffer = map_search(map, resource);

    if (buffer == NULL)
    {
        return &not_found;
    }

    const char *etag_client = strstr(headers, "If-None-Match: ");

    if (etag_client == NULL)
    {
        return buffer;
    }

    const char *etag_server = strstr(buffer->text, "ETag: ");

    if (etag_server == NULL)
    {
        return buffer;
    }

    etag_client += 15; // Length "If-None-Match: "
    etag_server += 6;  // Length "ETag: "
    // Length of SHA1 (20 digits) formatted as %02x = 40 + "" = 42
    if (strncmp(etag_server, etag_client, 42) == 0)
    {
        return &not_modified;
    }
    return buffer;
}

const buffer_t *static_get(const char *resource, const char *headers)
{
    if (resource == NULL)
    {
        return &bad_request;
    }
    return search(*resource ? resource : "index.html", headers);
}

const buffer_t *static_no_content(void)
{
    return &no_content;
}

const buffer_t *static_not_modified(void)
{
    return &not_modified;
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

