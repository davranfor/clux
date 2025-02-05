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
#include "static.h"
#include "writer.h"
#include "loader.h"

static map_t *schemas;

static void load_buffers(const char *path)
{
    char *file = file_read(path);

    if (file == NULL)
    {
        fprintf(stderr, "'%s' must exist\n", path);
        exit(EXIT_FAILURE);
    }
    printf("Loading '%s'\n", path);
    static_load(file);
    writer_load();
}

static int load_schemas(DIR *dir)
{
    const struct dirent *dirent;
    const char ext[] = ".json";

    while ((dirent = readdir(dir)))
    {
        size_t length = strlen(dirent->d_name);

        // File name must end with ".json"
        if ((length < sizeof(ext)) ||
            (strcmp(dirent->d_name + length - sizeof(ext) + 1, ext) != 0))
        {
            continue;
        }

        char path[255];

        if (snprintf(path, sizeof path, "schemas/%s", dirent->d_name) < 0)
        {
            fprintf(stderr, "'%s' is not a valid file name\n", dirent->d_name);
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
        if (map_insert(schemas, id, node) != node)
        {
            fprintf(stderr, "'%s' already mapped\n", id);
            json_delete(node);
            return 0;
        }
        printf("Loading '%s'\n", path);
    }
    return 1;
}

static void unload_schemas(void)
{
    map_destroy(schemas, json_free);
}

void loader_run(void)
{
    load_buffers("www/index.html");
    if (!(schemas = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    atexit(unload_schemas);

    DIR *dir;

    if (!(dir = opendir("schemas")))
    {
        fprintf(stderr, "'schemas' dir must exist\n");
        exit(EXIT_FAILURE);
    }

    int done = load_schemas(dir);

    closedir(dir);
    // Something went wrong
    if (!done)
    {
        exit(EXIT_FAILURE);
    }
}

