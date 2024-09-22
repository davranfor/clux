/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <clux/json.h>

static void print(const json_t *node, const char *path)
{
    char *text = json_encode(json_pointer(node, path));

    printf("\"%s\": %s\n", path, text ? text : "Not found");
    free(text);
}

int main(void)
{
    const char *str = "{\"\": 0, \"data\": [0, 1, 2], \"a~b\": 10, \"a/b\": 20}";
    json_t *node = json_parse(str, NULL);

    print(node, "");        // self node
    print(node, "/");       // empty key
    print(node, "/data");   // object
    print(node, "/data/1"); // array
    print(node, "/a~0b");   // special case 1: '~' must be escaped with '~0'
    print(node, "/a~1b");   // special case 2: '/' must be escaped with '~1'
    print(node, "/dummy");  // something that doesn't exists
    json_delete(node);
    return 0;
}

