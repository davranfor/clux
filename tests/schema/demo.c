/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <locale.h>
#include <stdlib.h>
#include <clux/json.h>
#include <clux/json_schema.h>

static json_t *parse_file(const char *path)
{
    json_error_t error;
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

static int on_validate(const json_t *target, const json_t *source,
    int event, void *data)
{
    const char **path = data;
    const char *msg[] =
    {
        "Warning. Unknown schema rule",
        "Invalid. Doesn't validate against schema rule",
        "Aborted. Malformed schema"
    };

    fprintf(stderr, "\nEvtMsg: %s\n", msg[event]);
    fprintf(stderr, "Node (%s)\n", path[0]);
    if (json_is_scalar(target))
    {
        json_write(target, stderr, 2);
    }
    fprintf(stderr, "Rule (%s)\n", path[1]);
    json_write(source, stderr, 2);
    return JSON_SCHEMA_CONTINUE;
}

int main(void)
{
    setlocale(LC_CTYPE, "");

    const char *path[] = {"test.json", "test.schema.json"};

    json_t *target = parse_file(path[0]);

    if (target == NULL)
    {
        exit(EXIT_FAILURE);
    }

    json_t *source = parse_file(path[1]);

    if (source == NULL)
    {
        json_delete(target);
        exit(EXIT_FAILURE);
    }

    if (!json_validate(target, source, on_validate, path))
    {
        fprintf(stderr, "%s doesn't validate against %s\n", path[0], path[1]);
    }
    json_delete(target);
    json_delete(source);
    return 0;
}

