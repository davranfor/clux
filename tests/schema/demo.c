/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <locale.h>
#include <stdlib.h>
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
    else
    {
        json_print(node);
    }
    return node;
}

static int on_validate(const json_schema_event_t *event, void *data)
{
    (void)data;

    const char *events[] =
    {
        "Warning. Unknown schema rule",
        "Failure. Doesn't validate against schema rule",
        "Aborted. Malformed schema",
    };

    fprintf(stderr, "\nEvent received\n");
    fprintf(stderr, "Type: %s\n", events[event->type]);
    fprintf(stderr, "Path: %s\n", event->path);
    fprintf(stderr, "Node: ");
    json_write_line(event->node, stderr);
    fprintf(stderr, "Rule: ");
    json_write_line(event->rule, stderr);
    return JSON_SCHEMA_CONTINUE;
}

int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");

    const char *path[] =
    {
        argc > 1 ? argv[1] : "test.json",
        argc > 2 ? argv[2] : "test.schema.json"
    };

    json_t *target = parse_file(path[0]);

    if (target == NULL)
    {
        exit(EXIT_FAILURE);
    }

    json_t *schema = parse_file(path[1]);

    if (schema == NULL)
    {
        json_delete(target);
        exit(EXIT_FAILURE);
    }
    if (!json_validate(target, schema, on_validate, NULL))
    {
        fprintf(stderr, "\n%s doesn't validate against %s\n", path[0], path[1]);
    }
    json_delete(target);
    json_delete(schema);
    return 0;
}

