/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <locale.h>
#include <clux/json_schema.h>

enum {TARGET, SCHEMA};

static json *parse(const char *path)
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

static void message(FILE *file, const char *title, const json *node)
{
    char *path = json_is_scalar(node)
        ? json_path(json_parent(node))
        : json_path(node);

    fprintf(file, "  Path: %s\n", path ? path : "$");
    free(path);

    char *test;

    if (json_is_scalar(node) && (test = json_encode(node)))
    {
        fprintf(file, "  %s: %s\n", title, test);
        free(test);
    }
}

/**
 * json_validate() callback function
 *
 * Events:
 * 0) Warning: A keyword is irrelevant for validation
 * 1) Invalid: Doesn't validate against a schema rule
 * 2) Aborted: Keyword with unexpected value (malformed schema)
 *
 * Return:
 *  0 to stop validating
 * !0 to continue
 *
 * Note:
 * Validation stops on event 2 even returning a non 0 value
 */
static int on_event(const json *target, const json *schema, int event, void *data)
{
    const char **path = data;
    const char *msg[] =
    {
        "Warning. Unknown schema rule",
        "Invalid. Doesn't validate against schema rule",
        "Aborted. Malformed schema"
    };

    fprintf(stderr, "\nEvtMsg: %s\n", msg[event]);
    fprintf(stderr, "Target: %s\n", path[TARGET]);
    message(stderr, "Node", target);
    fprintf(stderr, "Schema: %s\n", path[SCHEMA]);
    message(stderr, "Rule", schema);
    return 1; // Continue
}

static void validate(const json *target, const char **path)
{
    json *schema = parse(path[SCHEMA]);

    if (schema != NULL)
    {
        printf("\n%s:\n", path[TARGET]);
        json_print(target);
        printf("\n%s:\n", path[SCHEMA]);
        json_print(schema);
        if (json_validate(target, schema, on_event, path))
        {
            fprintf(stdout, "\n'%s' validated without errors\n", path[TARGET]);
        }
        else
        {
            fprintf(stderr, "\n'%s' doesn't validate\n", path[TARGET]);
        }
        json_free(schema);
    }
}

int main(void)
{
    setlocale(LC_CTYPE, "");

    const char *path[] = {"test.json", "test.schema.json"};
    json *target = parse(path[TARGET]);

    if (target != NULL)
    {
        validate(target, path);
        json_free(target);
    }
    return 0;
}

