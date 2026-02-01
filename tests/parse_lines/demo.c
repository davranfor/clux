/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <locale.h>
#include <sys/types.h>
#include <clux/json.h>

static json_t *parse_lines(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        perror("fopen");
        return NULL;
    }

    // getline() buffer
    char *str = NULL;

    // Lines are packed into array
    json_t *array = json_new_array();

    if (array == NULL)
    {
        perror("json_new_array");
        goto error;
    }

    json_error_t error; // Error handle is optional
    json_t *node;

    // getline() stuff
    ssize_t length = 0;
    size_t size = 0;
    int line = 0;

    while ((length = getline(&str, &size, file)) != -1)
    {
        line++;
        if (length <= 1)
        {
            continue;
        }
        if ((node = json_parse(str, &error)) == NULL)
        {
            error.line = line;
            fprintf(stderr, "%s\n", path);
            json_print_error(&error);
            goto error;
        }
        if (json_array_push_back(array, node) == NULL)
        {
            perror("json_array_push_back");
            goto error;
        }
    }
    goto clean;
error:
    json_delete(array);
    array = NULL;
clean:
    fclose(file);
    free(str);
    return array;
}

int main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, "C");

    json_t *node = parse_lines(argc > 1 ? argv[1] : "test.jsonl");

    json_print(node);
    json_free(node);
    return 0;
}

