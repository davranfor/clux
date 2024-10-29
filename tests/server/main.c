/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "config.h"
#include "schema.h"
#include "request.h"
#include "server.h"

static uint16_t port_number(const char *str)
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
    setlocale(LC_CTYPE, "");

    if ((argc == 2) && (strcmp(argv[1], "-h") == 0))
    {
        printf("Usage: %s [port]\n", argv[0]);
        return 0;
    }

    uint16_t port = argc > 1 ? port_number(argv[1]) : SERVER_PORT;

    if (port == 0)
    {
        fprintf(stderr, "Invalid port\n");
        exit(EXIT_FAILURE);
    }
    if (!schema_init())
    {
        exit(EXIT_FAILURE);
    }
    if (!request_init())
    {
        exit(EXIT_FAILURE);
    }
    server_init(port, request_ready, request_reply);
    return 0;
}

