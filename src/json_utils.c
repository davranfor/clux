/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_private.h"
#include "json_utils.h"

static int compare_node(const json_t *a, const json_t *b)
{
    if (a->type != b->type)
    {
        return a->type > b->type ? 1 : -1;
    }
    switch (a->type)
    {
        case JSON_STRING:
            return strcmp(a->string, b->string);
        case JSON_INTEGER:
        case JSON_REAL:
            return b->number > a->number ? -1 : a->number > b->number;
        default:
            return 0;
    }
}

static int compare_children(const json_t *a, const json_t *b)
{
    unsigned size = a->size < b->size ? a->size : b->size;

    for (unsigned i = 0; i < size; i++)
    {
        const json_t *c = a->child[i];
        const json_t *d = b->child[i];
        int cmp;

        if ((c->key != NULL) && (cmp = strcmp(c->key, d->key)))
        {
            return cmp;
        }
        if ((cmp = compare_node(c, d)))
        {
            return cmp;
        }
        if ((c->size || d->size) && (cmp = compare_children(c, d)))
        {
            return cmp;
        }
    }
    return b->size > a->size ? -1 : a->size != b->size;
}

/* Trusted comparators */

static int compare(const json_t *a, const json_t *b)
{
    int cmp;

    if ((cmp = compare_node(a, b)))
    {
        return cmp;
    }
    return a->size || b->size ? compare_children(a, b) : 0;
}

static int compare_by_key(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    return strcmp(a->key, b->key);
}

static int compare_by_value(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    return compare(a, b);
}

static int compare_by_key_value(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    int cmp = strcmp(a->key, b->key);

    return cmp ? cmp : compare(a, b);
}

/**
 * Compares two nodes
 * Returns
 * > 0 if a > b
 * < 0 if a < b
 * 0 otherwise
 */
int json_compare(const json_t *a, const json_t *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return a == b ? 0 : a == NULL ? -1 : 1;
    }

    int cmp;

    if ((cmp = compare_node(a, b)))
    {
        return cmp;
    }
    return a->size || b->size ? compare_children(a, b) : 0;
}

/* Search a key into an object (properties must be already sorted by key) */ 
json_t *json_search(const json_t *parent, const json_t *child,
    json_sort_callback callback)
{
    if ((parent == NULL) || (parent->size == 0) || (child == NULL))
    {
        return NULL;
    }
    if (callback == NULL)
    {
        callback = (parent->type == JSON_OBJECT) && (child->key != NULL)
            ? child->type == JSON_UNDEFINED ? compare_by_key : compare_by_key_value
            : compare_by_value;
    }

    json_t **node = bsearch(&child, parent->child, parent->size, sizeof child, callback);

    if (node != NULL)
    {
        return *node;
    }
    return NULL;
}

/* Sorts a json iterable using qsort, assign default callbacks if it is not provided */
void json_sort(json_t *node, json_sort_callback callback)
{
    if ((node == NULL) || (node->size <= 1))
    {
        return;
    }
    if (callback == NULL)
    {
        callback = node->type == JSON_OBJECT ? compare_by_key_value : compare_by_value;
    }
    qsort(node->child, node->size, sizeof *node->child, callback);
}

/* Reverses a json iterable */
void json_reverse(json_t *node)
{
    if ((node == NULL) || (node->size <= 1))
    {
        return;
    }

    unsigned lower = 0, upper = node->size - 1;

    while (lower < upper)
    {
        json_t *temp = node->child[lower];

        node->child[lower++] = node->child[upper];
        node->child[upper--] = temp;
    }
}

