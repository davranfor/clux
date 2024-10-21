/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <time.h>
#include <clux/json.h>

static int nulls_first(const void *pa, const void *pb)
{
    int a = json_is_null(*(json_t * const *)pa);
    int b = json_is_null(*(json_t * const *)pb);

    return a > b ? -1 : b > a ? 1 : json_compare_by_value(pa, pb);
}

int main(void)
{
    srand((unsigned)time(NULL));

    json_t *array = json_new_array();

    for (int i = 0; i <= 25; i++)
    {
        json_push_back(array, json_new_number(rand() % 100));
        if ((i % 5) == 0)
        {
            json_push_back(array, json_new_null());
        }
    }

    puts("Unsorted:");
    json_print(array);

    // Sort (default)
    puts("Sorted:");
    json_sort(array, NULL);
    json_print(array);

    // Sort by user-defined callback
    puts("Sorted (nulls first):");
    json_sort(array, nulls_first);
    json_print(array);

    puts("Reversed:");
    json_reverse(array);
    json_print(array);

    json_delete(array);
    return 0;
}

