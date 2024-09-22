/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <locale.h>
#include <clux/json.h>

static void parse_file(const char *path)
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
        json_delete(node);
    }
}

int main(void)
{
    setlocale(LC_CTYPE, "");
    parse_file("test.json");
    return 0;
}

