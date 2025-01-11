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
        json_print_error(&error);
    }
    else
    {
        json_print(node);
    }
    return node;
}

static int on_validate(const json_event_t *event, void *buffer)
{
    json_write_event(event, buffer, 128);
    return JSON_CONTINUE;
}

int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");

    const char *path[] =
    {
        argc > 1 ? argv[1] : "test.json",
        argc > 2 ? argv[2] : "test.schema.json"
    };

    puts(path[0]);

    json_t *target = parse_file(path[0]);

    if (target == NULL)
    {
        exit(EXIT_FAILURE);
    }

    puts(path[1]);

    json_t *schema = parse_file(path[1]);

    if (schema == NULL)
    {
        json_delete(target);
        exit(EXIT_FAILURE);
    }

    buffer_t events = {0};
    int rc = EXIT_SUCCESS;

    if (!json_validate(schema, target, NULL, on_validate, &events))
    {
        fprintf(stderr, "%s doesn't validate against %s\n", path[0], path[1]);
        rc = EXIT_FAILURE;
    }
    if (events.text != NULL)
    {
        fprintf(stderr, "%s", events.text);
        free(events.text);
    }
    json_delete(target);
    json_delete(schema);
    return rc;
}

