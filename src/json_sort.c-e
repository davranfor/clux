/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "json_private.h"

/**
 *  Swap a and b contents
 *  Returns a on success, NULL otherwise
 */
json *json_swap(json *a, json *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return NULL;
    }
    if ((a->parent || b->parent) && (!a->name != !b->name))
    {
        return NULL;
    }

    const json a_ = {.child = a->child, .name = a->name, .value = a->value};

    a->child = b->child; a->name = b->name; a->value = b->value;
    b->child = a_.child; b->name = a_.name; b->value = a_.value;
    return a;
}

static json *split(json *top)
{
    json *fast = top;
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
    if ((root != NULL) && (root->child != NULL))
    {
        json *node = sort(root->child, callback);

        root->child = node;
        // Reconnect nodes
        node->prev = NULL;
        while (node->next != NULL)
        {
            node->next->prev = node;
            node = node->next;
        }
    }
}

void json_reverse(json *root)
{
    if ((root != NULL) && (root->child != NULL))
    {
        json *node = root->child, *prev = NULL;

        while (node != NULL)
        {
            prev = node->prev;
            node->prev = node->next;
            node->next = prev;
            node = node->prev;
        }
        if (prev != NULL)
        {
            root->child = prev->prev;
        }
    }
}

