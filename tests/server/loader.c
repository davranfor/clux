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

static void unload(void)
{
    map_destroy(map, json_free);
}

static int load(DIR *schemas)
{
    struct dirent *dir;
    const char ext[] = ".schema.json"; 

    while ((dir = readdir(schemas)))
    {
        size_t length = strlen(dir->d_name);

        // File name must end with ".schema.json"
        if ((length < sizeof(ext)) ||
            (strcmp(dir->d_name + length - sizeof(ext) + 1, ext) != 0))
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
        printf("Loading '%s'\n", path);
    }
    return 1;
}

void loader_load(void)
{
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    json_schema_set_map(map);
    atexit(unload);

    DIR *schemas;

    if (!(schemas = opendir("schemas")))
    {
        fprintf(stderr, "'schemas' dir must exist\n");
        exit(EXIT_FAILURE);
    }

    int done = load(schemas);

    closedir(schemas);
    // Something went wrong
    if (!done)
    {
        exit(EXIT_FAILURE);
    }
}

