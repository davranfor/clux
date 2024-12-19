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
        "\u26D4 Aborted: Malformed schema",
        "\u2755 Notify: Annotation in schema",  
        "\u2757 Warning: Unknown schema rule",
        "\u274C Invalid: Doesn't validate against schema rule",
    };
    const char **path = data;

    fprintf(stderr, "\nTarget: %s\nSchema: %s\n", path[0], path[1]);
    fprintf(stderr, "Path: ");
    json_write_line(schema->path, stderr);
    fprintf(stderr, "Node: ");
    json_write_line(schema->node, stderr);
    fprintf(stderr, "Rule: ");
    json_write_line(schema->rule, stderr);
    fprintf(stderr, "%s\n", events[event]);
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
        fprintf(stderr, "\n%s doesn't validate against %s\n", path[0], path[1]);
    }
    json_delete(target);
    json_delete(schema);
    return 0;
}

