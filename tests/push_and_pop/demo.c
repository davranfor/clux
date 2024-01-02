/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
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

static void print(const char *function, const json *node,
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

    json *root = json_new_array("array");

    for (size_t i = 0; i < N; i++)
    {
        unsigned number = (unsigned)rand() % 90 + 10;
        unsigned offset = (unsigned)rand() % 5;
        json *node;

        switch (rand() % 9)
        {
            case 0:
                node = json_push_front(root, json_new_number(NULL, number));
                print("json_push_front", node, none, number);
                break;
            case 1:
                node = json_push_back(root, json_new_number(NULL, number));
                print("json_push_back", node, none, number);
                break;
            case 2:
                if ((node = json_at(root, offset)))
                {
                    json_push_before(node, json_new_number(NULL, number));
                }
                print("json_push_before", node, offset, number);
                break;
            case 3:
                if ((node = json_at(root, offset)))
                {
                    json_push_after(node, json_new_number(NULL, number));
                }
                print("json_push_after", node, offset, number);
                break;
            case 4:
                /* If offset is out of bounds it acts like json_push_back() */
                node = json_push_at(root, json_new_number(NULL, number), offset);
                print("json_push_at", node, offset, number);
                break;
            case 5:
                node = json_pop(json_at(root, offset));
                print("json_pop", node, offset, none);
                json_free(node);
                break;
            case 6:
                node = json_pop_front(root);
                print("json_pop_front", node, none, none);
                json_free(node);
                break;
            case 7:
                node = json_pop_back(root);
                print("json_pop_back", node, none, none);
                json_free(node);
                break;
            case 8:
                node = json_pop_at(root, offset);
                print("json_pop_at", node, offset, none);
                json_free(node);
                break;
            default:
                break;
        }
    }
    puts("");
    if (json_push_front(json_new_object(NULL), root))
    {
        json_push_after(root, json_new_integer("size", json_size(root)));
        root = json_parent(root);
        json_print(root);
    }
    json_free(root);
    return 0;
}

