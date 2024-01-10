#include "utils.h" 

json *parse_file(const char *path)
{
    json_error error;
    json *node = json_parse_file(path, &error);

    if (node == NULL)
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
    }
    return node;
}

