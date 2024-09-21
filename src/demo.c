#include <stdio.h>
#include <stdlib.h>
#include "json.h"

static json_t *root_node;

static void delete_root(void)
{
    json_delete(root_node);
}

static void set_root(json_t *root)
{
    root_node = root;
    atexit(delete_root);
}

static int walk(const json_t *node, size_t depth, void *data)
{
    (void)data;
    for (size_t i = 0; i < depth; i++)
    {
        printf("  ");
    }
    if (json_key(node) != NULL)
    {
        printf("%s: ", json_key(node));
    }
    switch (json_type(node))
    {
        case JSON_OBJECT:
            printf("{}");
            break;
        case JSON_ARRAY:
            printf("[]");
            break;
        case JSON_STRING:
            printf("\"%s\"", json_string(node));
            break;
        case JSON_INTEGER:
            printf("%.0f", json_number(node));
            break;
        case JSON_REAL:
            printf("%f", json_number(node));
            break;
        case JSON_TRUE:
            printf("true");
            break;
        case JSON_FALSE:
            printf("false");
            break;
        case JSON_NULL:
            printf("null");
            break;
        default:
            break;
    }
    printf("\n");
    return 1;
}

static json_t *object_prepend(json_t *parent, const char *key, json_t *child)
{
    if (!json_push(parent, JSON_HEAD, key, child))
    {
        json_delete(child);
        perror("object_prepend");
        exit(EXIT_FAILURE);
    }
    return child;
}

static json_t *object_append(json_t *parent, const char *key, json_t *child)
{
    if (!json_push(parent, JSON_TAIL, key, child))
    {
        json_delete(child);
        perror("object_append");
        exit(EXIT_FAILURE);
    }
    return child;
}

static json_t *array_prepend(json_t *parent, json_t *child)
{
    if (!json_push(parent, JSON_HEAD, child))
    {
        json_delete(child);
        perror("array_prepend");
        exit(EXIT_FAILURE);
    }
    return child;
}

static json_t *array_append(json_t *parent, json_t *child)
{
    if (!json_push(parent, JSON_TAIL, child))
    {
        json_delete(child);
        perror("array_append");
        exit(EXIT_FAILURE);
    }
    return child;
}

int main(void)
{
    json_t *object = json_new_object();

    if (object == NULL)
    {
        perror("json_new_object");
        exit(EXIT_FAILURE);
    }
    set_root(object);

    object_append(object, "", json_new_string("empty string"));

    const char *keys[] = {"zero", "one", "two", "three", "four"};
    enum {N = sizeof keys / sizeof keys[0]};
 
    for (int i = 1; i < N; i++)
    {
        object_append(object, keys[i], json_new_integer(i));
    }

    json_t *array1 = {object_append(object, "array", json_new_array())};

    for (int i = 1; i < N; i++)
    {
        array_append(array1, json_new_real(i));
    }
    array_prepend(array1, json_new_real(0));

    json_t *array2 = array_append(array1, json_new_array());

    for (int i = 0; i < N; i++)
    {
        array_append(array2, json_new_real(N + i));
    }

    object_prepend(object, "zero", json_new_integer(0));
    object_append(object, "five", json_new_integer(5));
    object_append(object, "true", json_new_boolean(1));
    object_append(object, "false", json_new_boolean(0));
    object_append(object, "null", json_new_null());
    object_append(object, "format", json_new_format("%s %s", "Hello", "world"));

    json_print(json_tail(object));
    json_print(json_tail(array1));

    json_print(object);

    json_walk(object, walk, NULL);

    printf("%d\n", json_equal(json_find(object, "array"), array1));
    printf("%d\n", json_equal(object, array1));

    for (unsigned i = 0, n = json_size(object); i < n; i++)
    {
        printf("- %s\n", json_key(json_at(object, i)));
    }

    json_print(json_pointer(object, "/array/5/2"));
    json_print(json_pointer(object, "/"));

/*
    json_parser_set_max_depth(3);
    json_t *test = json_parse("{\"array\": [1, \"hola\", [true, false, null], null, 3.14], \"object\": {}}", NULL);
    json_delete(json_pointer(test, "/array/2"), 2);
*/

    json_error_t error;
    json_t *test = json_parse_file("test.json", &error);

    if (test == NULL)
    {
        json_print_error(&error);
    }
    json_print(test);
    json_delete(test);
    return 0;
}

