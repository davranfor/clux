/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

int main(void)
{
/*
    json_t *a = json_parse("{\"a\": 1, \"b\": 2, \"c\": 3}", NULL);
    json_t *b = json_parse("{\"a\": 4, \"c\": 1, \"a\": 3, \"d\": 5, \"d\": 0}", NULL);

    puts("Before patch:");
    json_print(a);

    int patch = json_patch(a, b);

    puts("After patch:");
    json_print(a);

    json_unpatch(a, b, patch);
    puts("After unpatch:");
    json_print(a);

    json_delete(a);
    json_delete(b);
*/

    json_t *node = json_parse("[0, 1, 2, 3, 4, 5]", NULL);

    puts("Before move:");
    json_print(node);

    puts("After move 3 to 1:");
    json_move(node, 3, node, 1);
    json_print(node);

    puts("After move 1 to 3:");
    json_move(node, 1, 3);
    json_print(node);

    json_delete(node);

    return 0;
}

