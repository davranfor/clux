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

int json_is_unsigned(const json_t *node)
{
    return (node != NULL)
        && (node->type == JSON_INTEGER)
        && (node->number >= 0);
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

/* json_height helper */
static int tree_height(const json_t *node, size_t depth, void *height)
{
    (void)node;
    if (depth > *(size_t *)height)
    {
        *(size_t *)height = depth;
    }
    return 1;
}

/* Longest path from the root node to any leaf node */
size_t json_height(const json_t *node)
{
    size_t height = 0;

    json_walk(node, tree_height, &height);
    return height;
}

/* Number of nodes of an iterable */
unsigned json_size(const json_t *node)
{
    return node != NULL ? node->size : 0;
}

/* Number of nodes of an object */
unsigned json_properties(const json_t *node)
{
    return (node != NULL) && (node->type == JSON_OBJECT) ? node->size : 0;
}

/* Number of nodes of an array */
unsigned json_items(const json_t *node)
{
    return (node != NULL) && (node->type == JSON_ARRAY) ? node->size : 0;
}

/* Position in an object given a key */
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

/* Only for debugging purposes (the address may change on resize) */
const json_t * const *json_child(const json_t *node)
{
    if ((node != NULL) && (node->size > 0))
    {
        return (const json_t * const *)node->child;
    }
    return NULL;
}

/* First node of an iterable */
json_t *json_head(const json_t *node)
{
    if ((node != NULL) && (node->size > 0))
    {
        return node->child[0];
    }
    return NULL;
}

/* Last node of an iterable */
json_t *json_tail(const json_t *node)
{
    if ((node != NULL) && (node->size > 0))
    {
        return node->child[node->size - 1];
    }
    return NULL;
}

/* Returns node at position 'index' */
json_t *json_at(const json_t *node, size_t index)
{
    if ((node != NULL) && (node->size > index))
    {
        return node->child[index];
    }
    return NULL;
}

/* Locates a child by key */
json_t *json_find(const json_t *node, const char *key)
{
    if ((node == NULL) || (node->type != JSON_OBJECT) || (key == NULL))
    {
        return NULL;
    }
    for (unsigned index = 0; index < node->size; index++)
    {
        if (strcmp(node->child[index]->key, key) == 0)
        {
            return node->child[index];
        }
    }
    return NULL;
}

/* Locates a child by node */
json_t *json_locate(const json_t *parent, const json_t *child)
{
    if ((parent == NULL) || (child == NULL))
    {
        return NULL;
    }
    for (unsigned index = 0; index < parent->size; index++)
    {
        if (json_equal(parent->child[index], child))
        {
            return parent->child[index];
        }
    }
    return NULL;
}

/* Returns 1 if child is unique, 0 otherwise */
int json_is_unique(const json_t *parent, const json_t *child)
{
    if ((parent == NULL) || (child == NULL))
    {
        return 0;
    }
    for (unsigned i = 0; i < parent->size; i++)
    {
        if (parent->child[i] == child)
        {
            continue;
        }
        if (json_equal(parent->child[i], child))
        {
            return 0;
        }
    }
    return 1;
}

/* Returns 1 if all nodes are unique, 0 otherwise */
int json_unique_children(const json_t *node)
{
    if (node == NULL)
    {
        return 0;
    }
    for (unsigned i = 0; i < node->size; i++)
    {
        for (unsigned j = 0; j < i; j++)
        {
            if (json_equal(node->child[i], node->child[j]))
            {
                return 0;
            }
        }
    }
    return 1;
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

/* json_equal helper */
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

/* json_equal helper */
static int equal_children(const json_t *a, const json_t *b)
{
    for (unsigned i = 0; i < a->size; i++)
    {
        const json_t *c = a->child[i];
        const json_t *d = b->child[i];

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

/* Returns 1 if 'a' and 'b' and his children are equal, 0 otherwise */
int json_equal(const json_t *a, const json_t *b)
{
    if ((a != NULL) && (b != NULL))
    {
        if (!equal(a, b))
        {
            return 0;
        }
        return equal_children(a, b);
    }
    return 0;
}

/* json_walk recursive helper sending 'node' along with 'depth' and 'data' */
static int walk(const json_t *node, unsigned short depth,
    json_walk_callback callback, void *data)
{
    for (unsigned i = 0; i < node->size; i++)
    {
        int rc;

        if ((rc = callback(node->child[i], depth, data)) <= 0)
        {
            return rc;
        }
        if (node->child[i]->size > 0)
        {
            if ((rc = walk(node->child[i], depth + 1, callback, data)) <= 0)
            {
                return rc;
            }
        }
    }
    return 1;
}

/**
 * Traverses a json tree sendng all nodes to a callback
 * Uses a temporary parent in order to avoid checking the parent in helper
 */
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

