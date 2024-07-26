/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include "clib_math.h"
#include "json_list.h"

#define MIN_SIZE 8

struct json_list
{
    const json **data;
    size_t room;
    size_t size;
};

json_list *json_list_create(size_t room)
{
    json_list *list = malloc(sizeof *list);

    if (list != NULL)
    {
        list->size = 0;
        list->room = room <= MIN_SIZE ? MIN_SIZE : next_size(room);
        list->data = malloc(sizeof *list->data * list->room);
        if (list->data == NULL)
        {
            free(list);
            list = NULL;
        }
    }
    return list;
}

int json_list_add(json_list *list, const json *data)
{
    if ((list != NULL) && (data != NULL))
    {
        if (list->size == list->room)
        {
            size_t room = list->room * 2;
            const json **temp = realloc(list->data, sizeof *temp * room);

            if (temp == NULL)
            {
                return 0;
            }
            list->data = temp;
            list->room = room;
        }
        list->data[list->size++] = data;
        return 1;
    }
    return 0;
}

const json *json_list_at(const json_list *list, size_t index)
{
    if ((list != NULL) && (index < list->size))
    {
        return list->data[index];
    }
    return NULL;
}

const json **json_list_data(const json_list *list)
{
    return list != NULL ? list->data : NULL;
}

size_t json_list_size(const json_list *list)
{
    return list != NULL ? list->size : 0;
}

static void swap(const json **a, const json **b)
{
    const json *temp = *a;

    *a = *b;
    *b = temp;
}

static int partition(const json **list, int lo, int hi,
    json_sort_callback callback)
{
    const json *pivot = list[hi];
    int a = lo - 1;

    for (int b = lo; b < hi; b++)
    {
        int compare = callback(list[b], pivot);

        /**
         * Compare both values and list pointer positions in order
         * to preserve the relative order of equal elements
         */
        if ((compare < 0) || ((compare == 0) && (list[b] < pivot)))
        {
            a++;
            swap(&list[a], &list[b]);
        }
    }
    swap(&list[a + 1], &list[hi]);
    return a + 1;
}

static void sort(const json **list, int lo, int hi,
    json_sort_callback callback)
{
    if (lo < hi)
    {
        int part = partition(list, lo, hi, callback);

        sort(list, lo, part - 1, callback);
        sort(list, part + 1, hi, callback);
    }
}

void json_list_sort(json_list *list, json_sort_callback callback)
{
    if ((list != NULL) && (list->size > 1))
    {
        sort(list->data, 0, (int)list->size - 1, callback);
    }
}

void json_list_destroy(json_list *list)
{
    if (list != NULL)
    {
        free(list->data);
        free(list);
    }
}

