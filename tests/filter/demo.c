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
    json *root = json_new_array();

    json_push_back(root, json_new_string("foo"));
    json_push_back(root, json_new_number(1.0));
    json_push_back(root, json_new_integer(0));
    json_push_back(root, json_new_object());
    json_push_back(root, json_new_real(0.0));
    json_push_back(root, json_new_string("10000"));
    json_push_back(root, json_new_integer(1));
    json_push_back(root, json_new_array());
    json_push_back(root, json_new_null());
    json_push_back(root, json_parse("[9,8,7,6,2,3,4,5]", NULL));

    json_list *list = json_list_create(0);

    if (json_list_filter(list, root, filter_integers, NULL))
    {
        json_list_sort(list, JSON_LIST_SORT_BY_VALUE_ASC);
        print(json_list_data(list), json_list_size(list));
    }
    json_list_free(list);
    json_free(root);
    return 0;
}

