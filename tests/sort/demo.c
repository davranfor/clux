/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <time.h>
#include <clux/json.h>

static int nulls_first(const json *a, const json *b)
{
    return json_is_null(b) - json_is_null(a); 
}

int main(void)
{
    srand((unsigned)time(NULL));

    json *root = json_new_array();

    for (int i = 0; i <= 25; i++)
    {
        json_push_back(root, json_new_number(rand() % 100));
        if ((i % 5) == 0)
        {
            json_push_back(root, json_new_null());
        }
    }

    puts("Unsorted:");
    json_print(root);

    // Sort by predefined callback
    puts("Sorted by value (ascendent):");
    json_sort(root, json_compare_value_asc);
    json_print(root);

    // Sort by user-defined callback
    puts("Sorted by nulls first:");
    json_sort(root, nulls_first);
    json_print(root);

    puts("Reversed:");
    json_reverse(root);
    json_print(root);

    json_free(root);
    return 0;
}

