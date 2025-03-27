/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "schema.h"

static map_t *map;

static void schema_unload(void)
{
    map_destroy(map, json_free);
}

void schema_load(void)
{
    atexit(schema_unload);
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
}

int schema_push(const char *path)
{
    if (path == NULL)
    {
        return 0;
    }

    const char *extension = strrchr(path, '.');

    if (!extension || strcmp(extension, ".json"))
    {
        return 1;
    }
    printf("Loading '%s'\n", path);

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

    const json_t *temp = map_insert(map, id, node);

    if (temp != node)
    {
        fprintf(stderr, "'%s' %s\n", id, temp ? "already mapped" : strerror(errno));
        json_delete(node);
        return 0;
    }
    return 1;
}

