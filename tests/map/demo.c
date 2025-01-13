/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <time.h>
#include <clux/clib.h>
#include <clux/json.h>

static map_t *map;

static int print(void *node, size_t iter, void *data)
{
    (void)iter;
    (void)data;
    json_write_line(node, stdout);
    return 1;
}

static void destroy(void)
{
    map_destroy(map, json_free);
}

static void object_push_back(json_t *parent, const char *key, json_t *child)
{
    if (!json_object_push(parent, JSON_TAIL, key, child))
    {
        json_delete(parent);
        json_delete(child);
        perror("object_push_back");
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    srand((unsigned)time(NULL));
    if (!(map = map_create(0)))
    {
        perror("map_create");
        exit(EXIT_FAILURE);
    }
    atexit(destroy);

    enum {size = 100};
    json_t *node;

    for (size_t iter = 0; iter < size; iter++)
    {
        json_t *root = json_new_object();
        int code = rand() % size;

        object_push_back(root, "code", json_new_format("%05d", code));
        object_push_back(root, "func", json_new_string("insert"));
        node = map_insert(map, json_string(json_head(root)), root);
        if (node == NULL)
        {
            perror("map_insert");
            exit(EXIT_FAILURE);
        }
        if (node != root)
        {
            json_delete(root);
        }
    }
    for (size_t iter = 0; iter < size; iter++)
    {
        json_t *root = json_new_object();
        int code = rand() % size;

        object_push_back(root, "code", json_new_format("%05d", code));
        object_push_back(root, "func", json_new_string("upsert"));
        node = map_upsert(map, json_string(json_head(root)), root);
        if (node == NULL)
        {
            perror("map_upsert");
            exit(EXIT_FAILURE);
        }
        if (node != root)
        {
            json_delete(node);
        }
    }
    for (size_t iter = 0; iter < size; iter++)
    {
        char str[8];

        snprintf(str, sizeof str, "%05d", rand() % size);
        printf("Searching node %s: ", str);
        node = map_search(map, str);
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
        char str[8];

        snprintf(str, sizeof str, "%05d", rand() % size);
        printf("Deleting node %s: ", str);
        node = map_delete(map, str);
        if (node != NULL)
        {
            json_write_line(node, stdout);
            json_delete(node);
        }
        else
        {
            puts("Not found");
        }
    }
    puts("Printing map ...");
    map_walk(map, print, NULL);
    return 0;
}

