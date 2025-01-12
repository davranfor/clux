/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <clux/json.h>

static json_t *parse_file(const char *path)
{
    json_error_t error;
    json_t *node = json_parse_file(path, &error);

    if (node == NULL)
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
    }
    return node;
}

int main(void)
{
    json_t *root = parse_file("test.json");

    for (unsigned i = 0, n = json_is_array(root) ? json_size(root) : 0; i < n; i++)
    {
        const json_t *node = json_at(root, i);

        printf("Testing '%s' -> '%s' = %s\n",
            json_name(json_at(node, 1)),
            json_text(json_at(node, 1)),
            json_match(json_at(node, 1), json_string(json_at(node, 0))) ? "true" : "false"
        );
    }
    json_delete(root);
    return 0;
}

