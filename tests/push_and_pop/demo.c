/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/**
 * Testing push and pop family functions
 * -------------------------------------
 * 50 random insertions or deletions
 * Shows the result in a json object
 */

#include <stdlib.h>
#include <time.h>
#include <clux/json.h>

#define none (-1u)

static void print(const char *function, const json_t *node,
    unsigned offset, unsigned number)
{
    printf("{\"function\": \"%s\"", function);
    if (offset != none)
    {
        printf(", \"offset\": %u", offset);
    }
    if (number != none)
    {
        printf(", \"number\": %u", number);
    }
    printf(", \"result\": %s}\n", node != NULL ? "true" : "false");
}

int main(void)
{
    enum {N = 50};

    srand((unsigned)time(NULL));

    json_t *array = json_new_array();

    for (size_t i = 0; i < N; i++)
    {
        unsigned number = (unsigned)rand() % 90 + 10;
        unsigned offset = (unsigned)rand() % 5;
        json_t *node;

        switch (rand() % 6)
        {
            case 0:
                node = json_push_front(array, json_new_number(number));
                print("json_push_front", node, none, number);
                break;
            case 1:
                node = json_push_back(array, json_new_number(number));
                print("json_push_back", node, none, number);
                break;
            case 2:
                node = json_push(array, offset, json_new_number(number));
                print("json_push", node, offset, number);
                break;
            case 3:
                node = json_pop_front(array);
                print("json_pop_front", node, none, none);
                json_delete(node);
                break;
            case 4:
                node = json_pop_back(array);
                print("json_pop_back", node, none, none);
                json_delete(node);
                break;
            case 5:
                node = json_pop(array, offset);
                print("json_pop", node, offset, number);
                json_delete(node);
                break;
            default:
                break;
        }
    }
    puts("");

    json_t *root = json_new_object();

    if (json_push_front(root, "array", array))
    {
        json_push_back(root, "size", json_new_integer(json_size(array)));
        json_print(root);
        json_delete(root);
    }
    else
    {
        fprintf(stderr, "Something went wrong ...\n");
        json_delete(array);
        json_delete(root);
    }
    return 0;
}

