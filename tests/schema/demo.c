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

static int on_validate(const json_t *node, const json_t *rule,
    int event, void *data)
{
    const char **path = data;
    const char *msg[] =
    {
        "Warning. Unknown schema rule",
        "Invalid. Doesn't validate against schema rule",
        "Aborted. Malformed schema"
    };

    fprintf(stderr, "\nTarget: %s\nSchema: %s\n%s\n",
            path[0], path[1], msg[event]
    );
    if ((event == JSON_SCHEMA_INVALID) && json_is_scalar(node))
    {
        fprintf(stderr, "Node:\n");
        json_write(node, stderr, 2);
    }
    fprintf(stderr, "Rule\n");
    json_write(rule, stderr, 2);
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

