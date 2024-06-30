/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

int main(void)
{
    json *a = json_parse("{\"a\": 1, \"b\": 2, \"c\": 3}", NULL);
    json *b = json_parse("{\"a\": 3, \"c\": 1, \"d\": 0}", NULL);

    puts("Before patch:");
    json_print(a);

    int patch = json_patch(a, b);

    puts("After patch:");
    json_print(a);

    json_unpatch(a, b, patch);
    puts("After unpatch:");
    json_print(a);

    json_free(a);
    json_free(b);
    return 0;
}

