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
#include "common.h"
#include "loader.h"
#include "server.h"

int main(int argc, char *argv[])
{
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
    setlocale(LC_NUMERIC, "C");
    loader_run();
    printf("Server waiting on port %u\n", port);
    server_run(port);
    return 0;
}

