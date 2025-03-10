/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_private.h"
#include "json_utils.h"

/* Trusted comparison function for objects */
static int compare_by_key(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    return strcmp(a->key, b->key);
}

/* Default comparison function for objects */
int json_compare_by_key(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    if ((a->key != NULL) && (b->key != NULL))
    {
        return strcmp(a->key, b->key);
    }
    return 0;
}

/* Default comparison function for arrays */
int json_compare_by_value(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    if (a->type != b->type)
    {
        return a->type < b->type ? -1 : 1;
    }
    switch (a->type)
    {
        case JSON_STRING:
            return strcmp(a->string, b->string);
        case JSON_INTEGER:
        case JSON_REAL:
            return a->number < b->number ? -1 : a->number > b->number;
        default:
            return 0;
    }
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
            ? compare_by_key : json_compare_by_value;
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
        callback = node->type == JSON_OBJECT ? compare_by_key : json_compare_by_value;
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

