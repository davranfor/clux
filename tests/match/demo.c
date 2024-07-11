/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <locale.h>
#include <clux/json_regex.h>

static json * parse_file(const char *path)
{
    json_error error; // Error handle is optional
    json *node = json_parse_file(path, &error);

    if (node == NULL)
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
    }
    return node;
}

int main(void)
{
    setlocale(LC_CTYPE, "");

    json *root = parse_file("test.json");

    for (json *node = json_array_head(root); node != NULL; node = json_next(node))
    {
        if (json_is_string(json_at(node, 0)) && json_is_string(json_at(node, 1)))
        {
            printf("Testing %s -> %s = %s\n",
                json_name(json_at(node, 1)),
                json_string(json_at(node, 1)),
                json_match(json_at(node, 1), json_string(json_at(node, 0)))
                    ? "true"
                    : "false"
            );
        }
    }
    json_free(root);
    return 0;
}

