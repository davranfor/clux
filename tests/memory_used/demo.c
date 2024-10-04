/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <malloc/malloc.h>
#include <clux/clib.h>
#include <clux/json.h>

/* Print the size of a parsed JSON document in memory */

static int sum_memory_used(const json_t *node, size_t depth, void *data)
{
    (void)depth;

    size_t size = 0;

    if (json_key(node) != NULL)
    {
        size += malloc_size(json_key(node));
    }
    if (json_type(node) == JSON_STRING)
    {
        size += malloc_size(json_string(node));
    }
    if (json_size(node) > 0)
    {
        size += malloc_size(json_child(node));
    }
    size += malloc_size(node);
    *(size_t *)data += size;
    return 1;
}

static void print_memory_used(const char *path)
{
    char *file = file_read(path);

    if (file == NULL)
    {
        perror("file_read");
        exit(EXIT_FAILURE);
    }

    size_t file_size = strlen(file);

    json_error_t error;
    json_t *node = json_parse(file, &error);

    if (node != NULL)
    {
        size_t tree_size = 0;

        json_walk(node, sum_memory_used, &tree_size);
        printf("File size: %zu bytes\nTree size: %zu bytes\n",
            file_size, tree_size);
        json_delete(node);
    }
    else
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
    }
    free(file);
}

int main(void)
{
    print_memory_used("test.json");
    return 0;
}

