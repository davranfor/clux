/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <clux/json.h>
#include "schema.h"

static json_map_t *map;

static void map_destroy(void)
{
    json_map_destroy(map, json_free);
}

int schema_init(void)
{
    struct dirent *dir;
    DIR *schemas;

    schemas = opendir("schemas");
    if (schemas == NULL)
    {
        fprintf(stderr, "'schemas' dir must exist\n");
        return 0;
    }
    atexit(map_destroy);

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
        json_map_insert(map, id, node);
    }
    json_schema_set_map(map);
    closedir(schemas);
    return !fail;
}

