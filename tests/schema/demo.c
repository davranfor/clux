/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <locale.h>
#include <clux/json.h>

#define BUFFER_LIMIT 4096   // Don't write to buffer after this limit
#define ENCODE_MAX 128      // Max length of an event line

enum { CONTINUE, STOP };

static int on_notify(const json_t *rule)
{
    printf("Notification:\n");
    json_print(rule);
    return CONTINUE;
}

static int on_warning(const json_t *rule)
{
    fprintf(stderr, "Warning: Unknow rule '%s'\n", json_name(rule));
    return CONTINUE;
}

static int on_failure(const json_event_t *event, buffer_t *buffer)
{
    if (buffer->length > BUFFER_LIMIT)
    {
        buffer_write(buffer, "...\n");
        return STOP;
    }
    if (!json_write_event(buffer, event, ENCODE_MAX))
    {
        return STOP;
    }
    return CONTINUE;
}

static int on_error(const json_t *rule)
{
    fprintf(stderr, "Aborted: Malformed schema\n");
    json_write_line(rule, stderr);
    return STOP;
}

static int on_validate(const json_event_t *event, void *buffer)
{
    switch (event->type)
    {
        case JSON_NOTIFY:
            return on_notify(event->rule);
        case JSON_WARNING:
            return on_warning(event->rule);
        case JSON_FAILURE:
            return on_failure(event, buffer);
        case JSON_ERROR:
            return on_error(event->rule);
    }
    return STOP;
}

static int validate(const json_t *rules, const json_t *entry)
{
    buffer_t buffer = {0};
    int rc = EXIT_SUCCESS;

    if (!json_validate(rules, entry, NULL, on_validate, &buffer))
    {
        fprintf(stderr, "Doesn't validate against schema\n");
        rc = EXIT_FAILURE;
    }
    if (buffer.text != NULL)
    {
        fprintf(stderr, "Invalid rules:\n%s", buffer.text);
        free(buffer.text);
    }
    return rc;
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

    json_t *entry = parse_file(path[0]);

    if (entry == NULL)
    {
        exit(EXIT_FAILURE);
    }

    puts(path[1]);

    json_t *rules = parse_file(path[1]);

    if (rules == NULL)
    {
        json_delete(entry);
        exit(EXIT_FAILURE);
    }

    int rc = validate(rules, entry);

    json_delete(entry);
    json_delete(rules);
    return rc;
}

