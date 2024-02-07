/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include "json_private.h"

int JSON_SORT_BY_KEY_ASC(const json *a, const json *b)
{
    if ((a->name != NULL) && (b->name != NULL))
    {
        return strcmp(a->name, b->name);
    }
    return 0;
}

int JSON_SORT_BY_KEY_DESC(const json *a, const json *b)
{
    if ((a->name != NULL) && (b->name != NULL))
    {
        return strcmp(b->name, a->name);
    }
    return 0;
}

int JSON_SORT_BY_VALUE_ASC(const json *a, const json *b)
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

int JSON_SORT_BY_VALUE_DESC(const json *a, const json *b)
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

void json_sort(json *root, json_sort_callback callback)
{
    if ((root != NULL) && (root->size > 1))
    {
        json *node = sort(root->head, callback);

        root->head = node;
        node->prev = NULL;
        while (node->next != NULL)
        {
            node->next->prev = node;
            node = node->next;
        }
        root->tail = node;
    }
}

void json_reverse(json *root)
{
    if ((root != NULL) && (root->size > 1))
    {
        json *node = root->head, *prev;

        root->head = root->tail;
        root->tail = node;
        while (node != NULL)
        {
            prev = node->prev;
            node->prev = node->next;
            node->next = prev;
            node = node->prev;
        }
    }
}

