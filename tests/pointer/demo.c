/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

static void print(const json *node, const char *str)
{
    printf("Search '%s': '", str);
    json_write(json_pointer(node, str), 0, stdout);
    printf("'\n");
}

int main(void)
{
    const char *str = "{\"data\": [0, 1, 2], \"a~b\": 10, \"a/b\": 20}";
    json *node = json_parse(str, NULL);

    json_print(node);
    print(node, "/");       // root
    print(node, "/data");   // object
    print(node, "/data/1"); // array
    print(node, "/a~0b");   // special case 1: '~' must be escaped with '~0'
    print(node, "/a~1b");   // special case 2: '/' must be escaped with '~1'
    json_free(node);
    return 0;
}

