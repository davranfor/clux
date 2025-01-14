/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <locale.h>
#include <clux/json.h>

#define EVENTS_MAX_LENGTH 4096
#define EVENTS_MAX_ENCODE 128

static int on_validate(const json_event_t *event, void *buffer)
{
    if (event->type == JSON_WARNING)
    {
        fprintf(stderr, "Warning: Unknown schema rule '%s'\n",
            json_name(event->rule)
        );
        return JSON_CONTINUE;
    }
    if (event->type == JSON_ABORTED)
    {
        fprintf(stderr, "Aborted: Malformed schema\n"); 
        json_write_line(event->rule, stderr);
        return JSON_ABORT;
    }

    buffer_t *events = buffer;

    if (events->length > EVENTS_MAX_LENGTH)
    {
        buffer_write(events, "...\n");
        return JSON_ABORT;
    }
    if (!json_write_event(event, events, EVENTS_MAX_ENCODE))
    {
        return JSON_ABORT;
    }
    return JSON_CONTINUE;
}

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

int main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, "C");

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

