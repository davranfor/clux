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
#include "schema.h"

static map_t *map;

static void schema_map_destroy(void)
{
    map_destroy(map, json_free);
}

int schema_load_files(void)
{
    struct dirent *dir;
    DIR *schemas;

    if (!(schemas = opendir("schemas")))
    {
        fprintf(stderr, "'schemas' dir must exist\n");
        return 0;
    }
    if (!(map = map_create(0)))
    {
        perror("map_create");
        return 0;
    }
    json_schema_set_map(map);
    atexit(schema_map_destroy);

    const char ext[] = ".schema.json"; 
    int fail = 0;

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
            fail = 1;
            break;
        }

        json_error_t error;
        json_t *node = json_parse_file(path, &error);

        if (node == NULL)
        {
            fprintf(stderr, "%s\n", path);
            json_print_error(&error);
            fail = 1;
            break;
        }

        const char *id = json_string(json_find(node, "$id"));

        if (id == NULL)
        {
            fprintf(stderr, "Error mapping '%s' -> '$id' must exist\n", path);
            json_delete(node);
            fail = 1;
            break;
        }
        if (map_insert(map, id, node) != node)
        {
            fprintf(stderr, "'%s' already mapped\n", id);
            json_delete(node);
            fail = 1;
            break;
        }
    }
    closedir(schemas);
    return !fail;
}

