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

/**
 * Sorts a json iterable using qsort
 * If a callback comparison function is not provided:
 * - json_compare_by_key() is used for json objects
 * - json_compare_by_value() is used for json arrays
 */
void json_sort(json_t *node, json_sort_callback callback)
{
    if ((node != NULL) && (node->size > 1))
    {
        if (callback == NULL)
        {
            if (node->type == JSON_OBJECT)
            {
                callback = json_compare_by_key;
            }
            else
            {
                callback = json_compare_by_value;
            }
        }
        qsort(node->child, node->size, sizeof *node->child, callback);
    }
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

