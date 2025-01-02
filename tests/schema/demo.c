/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <locale.h>
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

static int on_validate(const json_schema_event_t *event, void *events)
{
    json_schema_write_event(event, events);
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

    buffer_t events = {0};

    if (!json_validate(target, schema, on_validate, &events))
    {
        fprintf(stderr, "%s doesn't validate against %s\n", path[0], path[1]);
    }
    if (events.text != NULL)
    {
        fprintf(stderr, "%s", events.text);
        free(events.text);
    }
    json_delete(target);
    json_delete(schema);
    return 0;
}

