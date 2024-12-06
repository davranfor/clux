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

static int on_validate(const json_schema_t *schema, int event, void *data)
{
    const char *events[] =
    {
        "Aborted: Malformed schema",
        "Notify: Annotation in schema",  
        "Warning: Unknown schema rule",
        "Invalid: Doesn't validate against schema rule",
    };
    const char **path = data;

    if (event != JSON_SCHEMA_NOTIFY)
    {
        fprintf(stderr, "\nTarget: %s\nSchema: %s\n", path[0], path[1]);
        fprintf(stderr, "Path: ");
        json_write(schema->path, stderr, 2);
        fprintf(stderr, "Node: ");
        json_write(schema->abbr.node, stderr, 2);
        fprintf(stderr, "Rule: ");
        json_write(schema->abbr.rule, stderr, 2);
        fprintf(stderr, "%s\n", events[event]);
    }
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
    if (!json_validate(target, schema, on_validate, path))
    {
        fprintf(stderr, "%s doesn't validate against %s\n", path[0], path[1]);
    }
    json_delete(target);
    json_delete(schema);
    return 0;
}

