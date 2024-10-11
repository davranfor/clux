/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <locale.h>
#include <clux/json.h>
#include <clux/json_schema.h>

static json_t *parse_file(const char *path)
{
    json_error_t error; // Error handle is optional
    json_t *node = json_parse_file(path, &error);

    if (node == NULL)
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
    }
    else
    {
        json_print(node);
    }
    return node;
}

int main(void)
{
    setlocale(LC_CTYPE, "");

    json_t *target = parse_file("test.json");
    json_t *source = parse_file("test.schema.json");

    json_validate(target, source, NULL, NULL);
    return 0;
}

