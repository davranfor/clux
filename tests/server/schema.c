/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <clux/json_writer.h>
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

map_t *schema_map(void)
{
    return map;
}

