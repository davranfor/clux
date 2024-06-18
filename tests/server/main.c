/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <clux/json.h>
#include "config.h"
#include "server.h"

static const char content_length_text[] = "Content-Length:";
static const char *header = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %zu\r\n\r\n";
static const char *delimiter = "\r\n\r\n";

enum {delimiter_length = 4};

static int request_done(const char *str, size_t size)
{
    const char *end = strstr(str, delimiter);

    if (end == NULL)
    {
        return 0;
    }
    end += delimiter_length;

    const char *mark = strstr(str, content_length_text);

    if (mark == NULL)
    {
        return !*end;
    }
    if (mark > end)
    {
        return 0;
    }

    size_t length = strtoul(mark + sizeof(content_length_text), NULL, 10);

    return size == (size_t)(end - str) + length + 1;
}

static void request_handle(struct poolfd *pool, char *buffer, size_t size)
{
    char *body = strstr(pool->data, delimiter);

    if (body == NULL)
    {
        return;
    }
    body += delimiter_length;

    json *node = json_parse(body, NULL);

    pool_reset(pool);
    if (node == NULL)
    {
        return;
    }
    body = json_encode(node);
    if (body != NULL)
    {
        size_t body_length = strlen(body);
        char head[256];

        snprintf(head, sizeof head, header, body_length);

        size_t head_length = strlen(head);

        if (head_length + body_length <= size)
        {
            memcpy(buffer, head, head_length);
            memcpy(buffer + head_length, body, body_length);
            pool_set(pool, buffer, head_length + body_length);
        }
        else if (pool_add(pool, head, head_length))
        {
            pool_add(pool, body, body_length);
        }
    }
    json_free(node);
    free(body);
}

static uint16_t string_to_uint16(const char *str)
{
    char *end;
    unsigned long result = strtoul(str, &end, 10);

    if ((result > 65535) || (end[strspn(end, " \f\n\r\t\v")] != '\0'))
    {
        return 0;
    }
    return (uint16_t)result;
}

int main(int argc, char *argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-h") == 0))
    {
        printf("Usage: %s [port]\n", argv[0]);
        return 0;
    }

    uint16_t port = argc > 1 ? string_to_uint16(argv[1]) : SERVER_PORT;

    if (port == 0)
    {
        fprintf(stderr, "Invalid port\n");
        exit(EXIT_FAILURE);
    }
    server_loop(port, request_done, request_handle);
    return 0;
}

