/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <clux/json.h>

static int test_query(const json *node, size_t depth, void *data)
{
    (void)data;

    const char *query[] =
    {
        "scalar",
        "string",
        "integer",
        "unique item",
        "array of scalars",
        "array of strings",
        "array of integers",
        "array of unique integers",
        "array of optional unique integers",
        "unique array of optional unique integers",
    };

    for (size_t i = 0; i < sizeof query / sizeof query[0]; i++)
    {
        const char *test = json_is(node, query[i]) ? "is" : "is not";
        char *text = json_encode(node);
        
        printf("%s %s '%s'\n", text ? text : "none", test, query[i]);
        free(text);
    }
    printf("\n");
    return depth == 0; // Only root and first child
}

int main(void)
{
    json *root = json_new_array();

    for (int i = 0; i < 2; i++)
    {
        json_push_back(root, json_new_number(i));
    }
    json_walk(root, test_query, NULL);
    json_set_integer(json_child(root), 1);
    json_walk(root, test_query, NULL);
    json_set_string(json_child(root), "zero");
    json_walk(root, test_query, NULL);
    while (json_delete(json_child(root)));
    json_walk(root, test_query, NULL);
    json_free(root);
    return 0;
}

