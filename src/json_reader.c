/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <string.h>
#include "json_private.h"
#include "json_reader.h"

const char *json_key(const json_t *node)
{
    if ((node != NULL) && (node->key != NULL))
    {
        return node->key;
    }
    return NULL;
}

const char *json_name(const json_t *node)
{
    if ((node != NULL) && (node->key != NULL))
    {
        return node->key;
    }
    return "";
}

const char *json_string(const json_t *node)
{
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return node->string;
    }
    return NULL;
}

const char *json_text(const json_t *node)
{
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return node->string;
    }
    return "";
}

double json_number(const json_t *node)
{
    if (node == NULL)
    {
        return 0.0;
    }
    switch (node->type)
    {
        case JSON_INTEGER:
        case JSON_REAL:
            return node->number;
        default:
            return 0.0;
    }
}

int json_boolean(const json_t *node)
{
    if ((node != NULL) && (node->type == JSON_TRUE))
    {
        return 1;
    }
    return 0;
}

int json_is_iterable(const json_t *node)
{
    if (node == NULL)
    {
        return 0;
    }
    switch (node->type)
    {
        case JSON_OBJECT:
        case JSON_ARRAY:
            return 1;
        default:
            return 0;
    }
}

int json_is_scalar(const json_t *node)
{
    if (node == NULL)
    {
        return 0;
    }
    switch (node->type)
    {
        case JSON_STRING:
        case JSON_INTEGER:
        case JSON_REAL:
        case JSON_TRUE:
        case JSON_FALSE:
        case JSON_NULL:
            return 1;
        default:
            return 0;
    }
}

int json_is_object(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_OBJECT);
}

int json_is_array(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_ARRAY);
}

int json_is_string(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_STRING);
}

int json_is_integer(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_INTEGER);
}

int json_is_real(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_REAL);
}

int json_is_number(const json_t *node)
{
    if (node == NULL)
    {
        return 0;
    }
    switch (node->type)
    {
        case JSON_INTEGER:
        case JSON_REAL:
            return 1;
        default:
            return 0;
    }
}

int json_is_boolean(const json_t *node)
{
    if (node == NULL)
    {
        return 0;
    }
    switch (node->type)
    {
        case JSON_TRUE:
        case JSON_FALSE:
            return 1;
        default:
            return 0;
    }
}

int json_is_true(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_TRUE);
}

int json_is_false(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_FALSE);
}

int json_is_null(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_NULL);
}

int json_is_root(const json_t *node)
{
    return (node != NULL)
        && !node->packed;
}

int json_is_child(const json_t *node)
{
    return (node != NULL)
        && (node->packed);
}

enum json_type json_type(const json_t *node)
{
    return node != NULL ? node->type : JSON_UNDEFINED;
}

unsigned json_size(const json_t *node)
{
    return node != NULL ? node->size : 0;
}

unsigned json_index(const json_t *node, const char *key)
{
    if ((node != NULL) && (node->type == JSON_OBJECT) && (key != NULL))
    {
        for (unsigned index = 0; index < node->size; index++)
        {
            if (!strcmp(node->child[index]->key, key))
            {
                return index;
            }
        }
    }
    return JSON_NOT_FOUND;
}

json_t *json_head(const json_t *node)
{
    if ((node != NULL) && (node->size > 0))
    {
        return node->child[0];
    }
    return NULL;
}

json_t *json_tail(const json_t *node)
{
    if ((node != NULL) && (node->size > 0))
    {
        return node->child[node->size - 1];
    }
    return NULL;
}

json_t *json_find(const json_t *node, const char *key)
{
    if ((node != NULL) && (node->type == JSON_OBJECT) && (key != NULL))
    {
        for (unsigned index = 0; index < node->size; index++)
        {
            if (strcmp(node->child[index]->key, key) == 0)
            {
                return node->child[index];
            }
        }
    }
    return NULL;
}

json_t *json_at(const json_t *node, size_t index)
{
    if ((node != NULL) && (node->size > index))
    {
        return node->child[index];
    }
    return NULL;
}

/**
 * Compares two nodes by value
 * Returns
 *  < 0 if a < b
 *  > 0 if a > b
 *  0 otherwise
 */
int json_compare(const json_t *a, const json_t *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return 0;
    }
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

static int equal(const json_t *a, const json_t *b)
{
    if ((a->type != b->type) || (a->size != b->size))
    {
        return 0;
    }
    switch (a->type)
    {
        case JSON_STRING:
            return strcmp(a->string, b->string) == 0;
        case JSON_INTEGER:
        case JSON_REAL:
            return a->number == b->number;
        default:
            return 1;
    }
}

static int equal_children(const json_t *a, const json_t *b)
{
    for (unsigned index = 0; index < a->size; index++)
    {
        const json_t *c = a->child[index];
        const json_t *d = b->child[index];

        if (!equal(c, d))
        {
            return 0;
        }
        if ((c->key != NULL) && strcmp(c->key, d->key))
        {
            return 0;
        }
        if ((c->size > 0) && !equal_children(c, d))
        {
            return 0;
        }
    }
    return 1;
}

int json_equal(const json_t *a, const json_t *b)
{
    if ((a != NULL) && (b != NULL)) 
    {
        if (!equal(a, b))
        {
            return 0;
        }
        if (a->size == 0)
        {
            return 1;
        }
        return equal_children(a, b);
    }
    return 0;
}

static int walk(const json_t *node, unsigned short depth,
    json_walk_callback callback, void *data)
{
    for (unsigned index = 0; index < node->size; index++)
    {
        int rc;

        if ((rc = callback(node->child[index], depth, data)) <= 0)
        {
            return rc;
        }
        if (node->child[index]->size > 0)
        {
            if ((rc = walk(node->child[index], depth + 1, callback, data)) <= 0)
            {
                return rc;
            }
        }
    }
    return 1;
}

int json_walk(const json_t *node, json_walk_callback callback, void *data)
{
    if (node != NULL) 
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        const json_t parent =
        {
            .child = (json_t *[]){(json_t *)node},
            .size = 1,
            .type = node->key ? JSON_OBJECT : JSON_ARRAY
        };
#pragma GCC diagnostic pop

        return walk(&parent, 0, callback, data);
    }
    return 0;
}

