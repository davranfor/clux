/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include "clib_math.h"
#include "json_private.h"
#include "json_list.h"

#define MIN_SIZE 8

struct json_list
{
    json **data;
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

json *json_list_push(json_list *list, json *data)
{
    if ((list != NULL) && (data != NULL))
    {
        if (list->size == list->room)
        {
            size_t room = list->room * 2;
            json **temp = realloc(list->data, sizeof *temp * room);

            if (temp == NULL)
            {
                return NULL;
            }
            list->data = temp;
            list->room = room;
        }
        list->data[list->size++] = data;
        return data;
    }
    return NULL;
}

json *json_list_pop(json_list *list)
{
    if ((list != NULL) && (list->size > 0))
    {
        list->size--;

        json *data = list->data[list->size];

        list->data[list->size] = NULL; 
        return data;
    }
    return NULL;
}

json *json_list_at(const json_list *list, size_t index)
{
    if ((list != NULL) && (index < list->size))
    {
        return list->data[index];
    }
    return NULL;
}

json **json_list_data(const json_list *list)
{
    return list != NULL ? list->data : NULL;
}

size_t json_list_size(const json_list *list)
{
    return list != NULL ? list->size : 0;
}

/**
 * Sends all nodes to a callback func providing list, node and user-data
 * Exit when all nodes are read or callback returns <= 0
 */
int json_list_filter(json_list *list, json *node,
    json_list_callback callback, void *data)
{
    if (list == NULL)
    {
        return 0;
    }

    size_t depth = 0;
    int flag = 1;

    while (node != NULL)
    {
        if (flag == 1)
        {
            int rc = callback(list, node, depth, data);
            
            if (rc <= 0)
            {
                return rc;
            }
        }
        if ((flag == 1) && (node->head != NULL))
        {
            node = node->head;
            depth++;
        }
        else if ((depth > 0) && (node->next != NULL))
        {
            node = node->next;
            flag = 1;
        }
        else if (depth-- > 0)
        {
            node = node->parent;
            flag = 0;
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

void json_list_sort(json_list *list, int (*callback)(const void *, const void *))
{
    if ((list != NULL) && (list->size > 1))
    {
        qsort(list->data, list->size, sizeof *list->data, callback);
    }
}

void json_list_reverse(json_list *list)
{
    if ((list != NULL) && (list->size > 1))
    {
        for (size_t a = 0, b = list->size - 1; a < b; a++, b--)
        {
            json *temp = list->data[a];

            list->data[a] = list->data[b];
            list->data[b] = temp;
        }
    }
}

void json_list_free(json_list *list)
{
    if (list != NULL)
    {
        free(list->data);
        free(list);
    }
}

