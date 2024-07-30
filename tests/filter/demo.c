/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

/**
 * json_list_filter() callback function
 *
 * Return:
 *  <= 0 to stop traversing
 *   > 0 to continue
 */
static int filter_integers(json_list *list, json *node, size_t depth, void *data)
{
    (void)depth;
    (void)data;
    if (json_is_integer(node))
    {
        return json_list_push(list, node) != NULL;
    }
    return 1;
}

static void print(json * const *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        json_print(data[i]);
    } 
}

int main(void)
{
    json *root = json_parse("[\"1\", 2, 0, {}, 1, 3.5, [9,8,7,6,2,3,4,5]]", NULL);
    json_list *list = json_list_create(0);

    if (json_list_filter(list, root, filter_integers, NULL))
    {
        json_list_sort(list, json_list_compare_value_asc);
        print(json_list_data(list), json_list_size(list));
    }
    json_list_free(list);
    json_free(root);
    return 0;
}

