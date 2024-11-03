/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "loader.h"

static map_t *map;

static void loader_destroy(void)
{
    map_destroy(map, json_free);
}

static int loader_fill(DIR *schemas)
{
    struct dirent *dir;
    const char ext[] = ".schema.json"; 

    while ((dir = readdir(schemas)))
    {
        const char *d_name_ext = strstr(dir->d_name, ext);

        if ((d_name_ext == NULL) || (strlen(d_name_ext) != sizeof(ext) -1))
        {
            continue;
        }

        char path[255];

        if (snprintf(path, sizeof path, "schemas/%s", dir->d_name) < 0)
        {
            fprintf(stderr, "'%s' is not a valid file name\n", dir->d_name);
            return 0;
        }

        json_error_t error;
        json_t *node = json_parse_file(path, &error);

        if (node == NULL)
        {
            fprintf(stderr, "%s\n", path);
            json_print_error(&error);
            return 0;
        }

        const char *id = json_string(json_find(node, "$id"));

        if (id == NULL)
        {
            fprintf(stderr, "Error mapping '%s' -> '$id' must exist\n", path);
            json_delete(node);
            return 0;
        }
        if (map_insert(map, id, node) != node)
        {
            fprintf(stderr, "'%s' already mapped\n", id);
            json_delete(node);
            return 0;
        }
    }
    return 1;
}

void loader_run(void)
{
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    json_schema_set_map(map);
    atexit(loader_destroy);

    DIR *schemas;

    if (!(schemas = opendir("schemas")))
    {
        fprintf(stderr, "'schemas' dir must exist\n");
        exit(EXIT_FAILURE);
    }

    int done = loader_fill(schemas);

    closedir(schemas);
    if (!done)
    {
        exit(EXIT_FAILURE);
    }
}

