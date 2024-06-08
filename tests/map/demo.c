/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <time.h>
#include <clux/json.h>

static json_map *map;

// cppcheck-suppress constParameterCallback
static int map_print(json *node, size_t iter, void *data)
{
    (void)iter;
    (void)data;
    json_write_line(node, stdout);
    return 1;
}

static void map_destroy(void)
{
    json_map_destroy(map, json_free);
}

int main(void)
{
    srand((unsigned)time(NULL));
    atexit(map_destroy);
    map = json_map_create(0);

    if (map == NULL)
    {
        perror("json_map_create");
        exit(EXIT_FAILURE);
    }

    enum {size = 100};
    json *node;

    for (size_t iter = 0; iter < size; iter++)
    {
        json *root = json_new_object(NULL);

        json_push_back(root, json_new_format("code", "%02d", rand() % size));
        json_push_back(root, json_new_string("func", "insert"));
        if (!(node = json_map_insert(map, json_string(json_child(root)), root)))
        {
            perror("json_map_insert");
            exit(EXIT_FAILURE);
        }
        if (node != root)
        {
            json_free(root);
        }
    }
    for (size_t iter = 0; iter < size; iter++)
    {
        json *root = json_new_object(NULL);

        json_push_back(root, json_new_format("code", "%02d", rand() % size));
        json_push_back(root, json_new_string("func", "upsert"));
        if (!(node = json_map_upsert(map, json_string(json_child(root)), root)))
        {
            perror("json_map_upsert");
            exit(EXIT_FAILURE);
        }
        if (node != root)
        {
            json_free(node);
        }
    }
    for (size_t iter = 0; iter < size; iter++)
    {
        char str[32];

        snprintf(str, sizeof str, "%02d", rand() % size);
        printf("Searching node %s: ", str);
        node = json_map_search(map, str);
        if (node != NULL)
        {
            json_write_line(node, stdout);
        }
        else
        {
            puts("Not found");
        }
    }
    for (size_t iter = 0; iter < size; iter++)
    {
        char str[32];

        snprintf(str, sizeof str, "%02d", rand() % size);
        printf("Deleting node %s: ", str);
        node = json_map_delete(map, str);
        if (node != NULL)
        {
            json_write_line(node, stdout);
            json_free(node);
        }
        else
        {
            puts("Not found");
        }
    }
    puts("Printing map ...");
    json_map_walk(map, map_print, NULL);
    return 0;
}

