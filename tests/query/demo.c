/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

static void query(json *node[], size_t size)
{
    const char *query[] =
    {
        "string",
        "integer",
        "array of strings",
        "array of integers",
        "array of unique integers",
        "array of optional unique integers",
    };

    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < sizeof query / sizeof query[0]; j++)
        {
            printf("Query '%s' for node: '", query[j]);
            json_write(node[i], 0, stdout);
            printf("' is %s\n", json_is(node[i], query[j]) ? "true" : "false");
        }
        printf("----------------------------------------------------------\n");
    }
}

int main(void)
{
    json *root, *tail = NULL;

    root = json_new_array(NULL);
    for (int i = 0; i < 3; i++)
    {
        tail = json_push_fast(root, tail, json_new_integer(NULL, i));
    }

    json *node[] = {json_child(root), root};
    enum {size = sizeof node / sizeof node[0]};

    query(node, size);
    json_set_integer(node[0], 1);
    query(node, size);
    json_set_string(node[0], "zero");
    query(node, size);
    while ((node[0] = json_delete(node[0])));
    query(node, size);
    json_free(root);
    return 0;
}

