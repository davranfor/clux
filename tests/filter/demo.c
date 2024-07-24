/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

/**
 * json_walk() callback function
 *
 * Return:
 *  0 to stop traversing
 * !0 to continue
 */
static int filter_integers(const json *node, int depth, void *data)
{
    (void)depth;
    if (json_is_integer(node))
    {
        return json_list_add(data, node);
    }
    return 1;
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
    json_push_back(root, json_new_integer(2));
    json_push_back(root, json_new_null());
    json_push_back(root, json_parse("[3,4,5,6,7,8,9]", NULL));

    json_list *list = json_list_create(0);

    if ((list != NULL) && json_walk(root, filter_integers, list))
    {
        for (size_t i = 0, n = json_list_size(list); i < n; i++)
        {
            json_print(json_list_at(list, i));
        } 
    }
    json_list_destroy(list);
    json_free(root);
    return 0;
}

