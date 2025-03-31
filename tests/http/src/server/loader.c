/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "static.h"
#include "schema.h"
#include "loader.h"

static int load(const char *path, int (*func)(const char *))
{
    DIR *dir;

    if (!(dir = opendir(path)))
    {
        fprintf(stderr, "'%s' dir must exist\n", path);
        exit(EXIT_FAILURE);
    }

    const struct dirent *dirent;
    int rc = 1;

    while ((dirent = readdir(dir)))
    {
        char file[256];
        size_t length = (size_t)snprintf(file, sizeof file, "%s/%s", path, dirent->d_name);

        if (length >= sizeof file)
        {
            fprintf(stderr, "'%s' is not a valid file name\n", dirent->d_name);
            rc = 0;
            break;
        }
        if (!func(file))
        {
            rc = 0;
            break;
        }
    }
    closedir(dir);
    return rc;
}

void loader_load(void)
{
    static_load();
    schema_load();
    if (!load("schemas", schema_add) || !load("www", static_add))
    {
        exit(EXIT_FAILURE);
    }
}

void loader_reload(void)
{
    static_reload();
    schema_reload();
    if (!load("schemas", schema_add) || !load("www", static_add))
    {
        exit(EXIT_FAILURE);
    }
}

