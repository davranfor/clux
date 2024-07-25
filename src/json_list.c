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
        list->room = room <= MIN_SIZE ? MIN_SIZE : next_pow2(room);
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

void json_list_destroy(json_list *list)
{
    if (list != NULL)
    {
        free(list->data);
        free(list);
    }
}

