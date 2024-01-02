/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <time.h>
#include <clux/json.h>

int main(void)
{
    srand((unsigned)time(NULL));

    json *root = json_new_array(NULL);

    for (int i = 0; i < 25; i++)
    {
        json_push_back(root, json_new_integer(NULL, rand() % 100));
    }
    puts("Unsorted:");
    json_print(root);
    puts("Sorted:");
    json_sort(root, json_compare);
    json_print(root);
    puts("Reversed:");
    json_reverse(root);
    json_print(root);
    json_free(root);
    return 0;
}

