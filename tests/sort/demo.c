/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <time.h>
#include <clux/json.h>
#include <clux/json_private.h>

static void array_push_back(json_t *parent, json_t *child)
{
    if (!json_array_push_back(parent, child))
    {
        json_delete(parent);
        json_delete(child);
        perror("array_push_back");
        exit(EXIT_FAILURE);
    }
}

static int nulls_first(const void *pa, const void *pb)
{
    json_t *a = *(json_t * const *)pa;
    json_t *b = *(json_t * const *)pb;

    if ((a->type != JSON_NULL) && (b->type != JSON_NULL))
    {
        return json_compare(a, b);
    }
    return a->type != JSON_NULL ? 1 : -1;
}

int main(void)
{
    srand((unsigned)time(NULL));

    json_t *object = json_parse("{\"b\": 1, \"a\": 0, \"c\": 2, \"d\": 3}", NULL);

    if (object == NULL)
    {
        perror("json_parse");
        exit(EXIT_FAILURE);
    }
    puts("Search key 'b':");
    json_sort(object, NULL);
    json_print(json_search(object, &(json_t){.key = "b"}, NULL));
    json_delete(object);

    json_t *array = json_new_array();

    for (int i = 0; i <= 25; i++)
    {
        if ((i % 5) != 0)
        {
            array_push_back(array, json_new_number(rand() % 100));
        }
        else
        {
            array_push_back(array, json_new_null());
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

