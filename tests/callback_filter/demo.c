/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

/**
 * json_walk() callback function
 *
 * Return:
 * <= 0 to stop traversing
 *  > 0 to continue
 */
static int sum_numbers_until_null(const json *node, int depth, void *data)
{
    (void)depth;
    if (json_is_number(node)) // Returns true for integer, real and double
    {
        *(double *)data += json_number(node);
    }
    return !json_is_null(node);
}

int main(void)
{
    json *root, *tail;

    root = json_new_array(NULL);
    tail = json_push_front(root, json_new_string(NULL, "foo"));   // Not computed
    tail = json_push_after(tail, json_new_integer(NULL, -39));
    tail = json_push_after(tail, json_new_real(NULL, 42));
    tail = json_push_after(tail, json_new_string(NULL, "bar"));   // Not computed
    tail = json_push_after(tail, json_new_object(NULL));          // Not computed
    tail = json_push_after(tail, json_new_number(NULL, 0));
    tail = json_push_after(tail, json_new_string(NULL, "10000")); // Not computed
    tail = json_push_after(tail, json_new_format(NULL, "%d", 1)); // Not computed
    tail = json_push_after(tail, json_new_double(NULL, 0.14));
    tail = json_push_after(tail, json_new_boolean(NULL, 1));      // Not computed
    tail = json_push_after(tail, json_new_null(NULL));            // Stop here
    tail = json_push_after(tail, json_new_integer(NULL, 100));

    double sum = 0.0;

    json_walk(root, sum_numbers_until_null, &sum);
    printf("Sum = %.2f\n", sum); // should be 3.14
    json_free(root);
    return 0;
}

