/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include "json_private.h"
#include "json_sort.h"

int json_compare_key_asc(const json *a, const json *b)
{
    if ((a->name != NULL) && (b->name != NULL))
    {
        return strcmp(a->name, b->name);
    }
    return 0;
}

int json_compare_key_desc(const json *a, const json *b)
{
    if ((a->name != NULL) && (b->name != NULL))
    {
        return strcmp(b->name, a->name);
    }
    return 0;
}

int json_compare_value_asc(const json *a, const json *b)
{
    if (a->type != b->type)
    {
        return a->type > b->type ? 1 : -1;
    }
    if (a->type != JSON_STRING)
    {
        return
            a->value.number < b->value.number ? -1 :
            a->value.number > b->value.number;
    }
    return strcmp(a->value.string, b->value.string);
}

int json_compare_value_desc(const json *a, const json *b)
{
    if (a->type != b->type)
    {
        return b->type > a->type ? 1 : -1;
    }
    if (a->type != JSON_STRING)
    {
        return
            b->value.number < a->value.number ? -1 :
            b->value.number > a->value.number;
    }
    return strcmp(b->value.string, a->value.string);
}

static json *split(json *top)
{
    const json *fast = top;
    json *slow = top;

    while ((fast->next != NULL) && (fast = fast->next->next))
    {
        slow = slow->next;
    }

    json *mid = slow->next;

    slow->next = NULL;
    return mid;
}

static json *merge(json *a, json *b, json_sort_callback callback)
{
    json *top = NULL;
    json **ab = &top;

    while ((a != NULL) && (b != NULL))
    {
        if (callback(a, b) <= 0)
        {
            *ab = a; a = a->next;
        }
        else
        {
            *ab = b; b = b->next;
        }
        ab = &((*ab)->next);
    }
    *ab = a != NULL ? a : b;
    return top;
}

static json *sort(json *top, json_sort_callback callback)
{
    if ((top == NULL) || (top->next == NULL))
    {
        return top;
    }

    json *mid = split(top);

    top = sort(top, callback);
    mid = sort(mid, callback);
    return merge(top, mid, callback);
}

void json_sort(json *parent, json_sort_callback callback)
{
    if ((parent != NULL) && (parent->size > 1))
    {
        json *node = sort(parent->head, callback);

        parent->head = node;
        parent->head->prev = NULL;
        while (node->next != NULL)
        {
            node->next->prev = node;
            node = node->next;
        }
        parent->tail = node;
    }
}

void json_reverse(json *parent)
{
    if ((parent != NULL) && (parent->size > 1))
    {
        json *node = parent->head, *prev;

        parent->head = parent->tail;
        parent->tail = node;
        while (node != NULL)
        {
            prev = node->prev;
            node->prev = node->next;
            node->next = prev;
            node = node->prev;
        }
    }
}

