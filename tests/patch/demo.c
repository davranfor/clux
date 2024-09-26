/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

int main(void)
{
    json_t *a = json_parse("{\"a\": 4, \"c\": 1, \"a\": 3, \"d\": 5, \"d\": 0}", NULL);
    json_t *b = json_parse("{\"a\": 1, \"b\": 2, \"c\": 3}", NULL);

    puts("Before patch:");
    json_print(b);

    int patch = json_patch(a, b);

    puts("After patch:");
    json_print(b);

    json_unpatch(a, b, patch);
    puts("After unpatch:");
    json_print(b);

    json_delete(a);
    json_delete(b);

/*
    json_t *a = json_parse("{\"a\": 0, \"b\": 1, \"c\": 2, \"d\": 3, \"e\": 4, \"f\": 5}", NULL);
    json_t *b = json_parse("[0, 1, 2, 3, 4]", NULL);

    json_move(b, 3, 1);
    json_print(b);

    json_delete(a);
    json_delete(b);
*/

    return 0;
}

