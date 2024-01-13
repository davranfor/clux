/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

int main(void)
{
    json *root = json_new_array(NULL);

    for (int i = 0; i <= 20; i++)
    {
        json_push_back(root, json_new_integer(NULL, i));
    }
    json_print(root);
    // Delete first 10 elements
    for (int i = 0; i < 10; i++)
    {
        json_delete(json_child(root));
    }
    json_print(root);
    json_free(root);
    return 0;
}

