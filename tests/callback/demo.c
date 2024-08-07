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
 *  <= 0 to stop traversing
 *   > 0 to continue
 */
static int sum_numbers_until_null(const json *node, size_t depth, void *data)
{
    (void)depth;
    if (json_is_number(node))
    {
        *(double *)data += json_number(node);
    }
    return !json_is_null(node);
}

int main(void)
{
    json *root = json_new_array();

    json_push_back(root, json_new_string("foo"));   // Not computed
    json_push_back(root, json_new_number(-39));
    json_push_back(root, json_new_integer(42));
    json_push_back(root, json_new_object());        // Not computed
    json_push_back(root, json_new_real(0.0));
    json_push_back(root, json_new_string("10000")); // Not computed
    json_push_back(root, json_new_format("%d", 1)); // Not computed
    json_push_back(root, json_new_real(0.14));
    json_push_back(root, json_new_boolean(1));      // Not computed
    json_push_back(root, json_new_null());          // Stop here
    json_push_back(root, json_new_integer(100));    // Not computed

    double sum = 0.0;

    json_walk(root, sum_numbers_until_null, &sum);
    printf("Sum = %.2f\n", sum); // should be 3.14
    json_free(root);
    return 0;
}

