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

/* Sorts a json iterable using qsort, assign default callbacks if not provided */
void json_sort(json_t *node, json_sort_callback callback)
{
    if ((node != NULL) && (node->size > 1))
    {
        if (callback == NULL)
        {
            callback = node->type == JSON_OBJECT ? compare_by_key : json_compare_by_value;
        }
        qsort(node->child, node->size, sizeof *node->child, callback);
    }
}

/* Search a key into an object (properties must be already sorted by key) */ 
json_t *json_search(const json_t *node, const char *key)
{
    if ((node == NULL) || (node->type != JSON_OBJECT) || (key == NULL))
    {
        return NULL;
    }

    unsigned lower = 0, upper = node->size;

    while (lower < upper)
    {
        unsigned index = lower + (upper - lower) / 2;
        int cmp = strcmp(key, node->child[index]->key);

        if (cmp < 0)
        {
            upper = index;
        }
        else if (cmp > 0)
        {
            lower = index + 1;
        }
        else
        {
            return node->child[index];
        }
    }
    return NULL;
}

/* Reverses a json iterable */
void json_reverse(json_t *node)
{
    if ((node != NULL) && (node->size > 1))
    {
        for (unsigned a = 0, b = node->size - 1; a < b; a++, b--)
        {
            json_t *temp = node->child[a];

            node->child[a] = node->child[b];
            node->child[b] = temp;
        }
    }
}

