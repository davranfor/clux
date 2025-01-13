/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clib_string.h"
#include "clib_math.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_writer.h"

/* Returns a new allocated json object */
json_t *json_new_object(void)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_OBJECT;
    }
    return node;
}

/* Returns a new allocated json array */
json_t *json_new_array(void)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_ARRAY;
    }
    return node;
}

/* Returns a new allocated json string */
json_t *json_new_string(const char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    char *string = string_clone(str);

    if (string == NULL)
    {
        return NULL;
    }

    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_STRING;
        node->string = string;
    }
    else
    {
        free(string);
    }
    return node;
}

/* Returns a new allocated json string using printf-style formatting */
json_t *json_new_format(const char *fmt, ...)
{
    if (fmt == NULL)
    {
        return NULL;
    }

    va_list args;

    va_start(args, fmt);

    char *string = string_vprint(fmt, args);

    va_end(args);

    if (string == NULL)
    {
        return NULL;
    }

    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_STRING;
        node->string = string;
    }
    else
    {
        free(string);
    }
    return node;
}

/**
 * Returns a new allocated
 * - json integer if 'number' can be represented as a safe integer
 * - json real otherwise
 * Safe integers are numbers within a range of -2^52 to +2^52 (inclusive)
 */
json_t *json_new_integer(double number)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->number = trunc(number);
        node->type = IS_SAFE_INTEGER(node->number)
            ? JSON_INTEGER
            : JSON_REAL;
    }
    return node;
}

/* Returns a new allocated json real */
json_t *json_new_real(double number)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_REAL;
        node->number = number;
    }
    return node;
}

/* Returns a new allocated json boolean */
json_t *json_new_boolean(int number)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = number != 0
            ? JSON_TRUE
            : JSON_FALSE;
    }
    return node;
}

/* Returns a new allocated json null */
json_t *json_new_null(void)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_NULL;
    }
    return node;
}

/* Modifies/sets the key and returns itself */
json_t *json_set_key(json_t *node, const char *str)
{
    if ((node == NULL) || (str == NULL) || (node->packed && !node->key))
    {
        return NULL;
    }

    char *key = string_clone(str);

    if (key == NULL)
    {
        return NULL;
    }
    free(node->key);
    node->key = key;
    return node;
}

/* Removes the key and returns itself */
json_t *json_unset_key(json_t *node)
{
    if (!node || node->packed)
    {
        return NULL;
    }
    free(node->key);
    node->key = NULL;
    return node;
}

/* Cleanup the passed node */
static void clear(json_t *node)
{
    switch (node->type)
    {
        case JSON_OBJECT:
        case JSON_ARRAY:
            json_delete_children(node);
            break;
        case JSON_STRING:
            free(node->string);
            node->string = NULL;
            break;
        case JSON_INTEGER:
        case JSON_REAL:
            node->number = 0.0;
            break;
    }
}

/* Modifies and returns a json object */
json_t *json_set_object(json_t *node)
{
    if (node != NULL)
    {
        clear(node);
        node->type = JSON_OBJECT;
    }
    return node;
}

/* Modifies and returns a json array */
json_t *json_set_array(json_t *node)
{
    if (node != NULL)
    {
        clear(node);
        node->type = JSON_ARRAY;
    }
    return node;
}

/* Modifies and returns a json string */
json_t *json_set_string(json_t *node, const char *str)
{
    if ((node == NULL) || (str == NULL))
    {
        return NULL;
    }

    char *string = string_clone(str);

    if (string == NULL)
    {
        return NULL;
    }
    clear(node);
    node->type = JSON_STRING;
    node->string = string;
    return node;
}

/* Modifies and returns a json string using printf-style formatting */
json_t *json_set_format(json_t *node, const char *fmt, ...)
{
    if ((node == NULL) || (fmt == NULL))
    {
        return NULL;
    }

    va_list args;

    va_start(args, fmt);

    char *string = string_vprint(fmt, args);

    va_end(args);

    if (string == NULL)
    {
        return NULL;
    }
    clear(node);
    node->type = JSON_STRING;
    node->string = string;
    return node;
}

/**
 * Modifies and returns
 * - json integer if 'number' can be represented as a safe integer
 * - json real otherwise
 * Safe integers are numbers within a range of -2^52 to +2^52 (inclusive)
 */
json_t *json_set_integer(json_t *node, double number)
{
    if (node != NULL)
    {
        clear(node);
        node->number = trunc(number);
        node->type = IS_SAFE_INTEGER(node->number)
            ? JSON_INTEGER
            : JSON_REAL;
    }
    return node;
}

/* Modifies and returns a json real */
json_t *json_set_real(json_t *node, double number)
{
    if (node != NULL)
    {
        clear(node);
        node->type = JSON_REAL;
        node->number = number;
    }
    return node;
}

/* Modifies and returns a json boolean */
json_t *json_set_boolean(json_t *node, int number)
{
    if (node != NULL)
    {
        clear(node);
        node->type = number != 0
            ? JSON_TRUE
            : JSON_FALSE;
    }
    return node;
}

/* Modifies and returns a json null */
json_t *json_set_null(json_t *node)
{
    if (node != NULL)
    {
        clear(node);
        node->type = JSON_NULL;
    }
    return node;
}

/* Push 'child' into 'parent' at position 'index' with an optional 'key' */
static json_t *push(json_t *parent, unsigned index, const char *name,
    json_t *child)
{
    // Can't push itself nor a node with parent
    if ((parent == child) || child->packed)
    {
        return NULL;
    }
    // If index is -1 push to the back
    if (index > parent->size)
    {
        if (index != -1u)
        {
            return NULL;
        }
        index = parent->size;
    }
 
    char *key = NULL;

    if ((name != NULL) && !(key = string_clone(name)))
    {
        return NULL;
    }

    // Space for inner nodes is incremented when size is a power of 2
    unsigned size = next_size(parent->size);

    if (size > parent->size) 
    {
        json_t **temp = realloc(parent->child, sizeof(*temp) * size);

        if (temp == NULL)
        {
            free(key);
            return NULL;
        }
        parent->child = temp;
    }
    if (index < parent->size)
    {
        memmove(parent->child + index + 1,
                parent->child + index,
                sizeof(*parent->child) * (parent->size - index));
    }
    if ((parent->type == JSON_ARRAY) || (key != NULL))
    {
        free(child->key);
        child->key = key;
    }
    child->packed = 1;
    parent->child[index] = child;
    parent->size++;
    return child;    
}

/* Push 'child' into 'parent' at position 'index' with a 'key' */ 
json_t *json_object_push(json_t *parent, size_t index, const char *key,
    json_t *child)
{
    if ((parent == NULL) || (key == NULL) || (child == NULL))
    {
        return NULL;
    }
    if (parent->type == JSON_OBJECT)
    {
        return push(parent, (unsigned)index, key, child);
    }
    return NULL;
}

/* Push 'child' into 'parent' at position 'index' if parent is an array */
json_t *json_array_push(json_t *parent, size_t index, json_t *child)
{
    if ((parent == NULL) || (child == NULL))
    {
        return NULL;
    }
    if (parent->type == JSON_ARRAY)
    {
        return push(parent, (unsigned)index, NULL, child);
    }
    return NULL;
}

/* Push 'child' into 'parent' at position 'index' */ 
json_t *json_push_at(json_t *parent, size_t index, json_t *child)
{
    if ((parent == NULL) || (child == NULL))
    {
        return NULL;
    }
    if ((parent->type == JSON_ARRAY)
    || ((parent->type == JSON_OBJECT) && (child->key != NULL)))
    {
        return push(parent, (unsigned)index, NULL, child);
    }
    return NULL;
}

/* Pop node at position 'index' */
static json_t *pop(json_t *parent, unsigned index)
{
    // If index is -1 pop from the back
    if (index >= parent->size)
    {
        if (index != -1u)
        {
            return NULL;
        }
        index = parent->size - 1;
    }

    json_t *child = parent->child[index];

    if (index < parent->size - 1)
    {
        memmove(parent->child + index,
                parent->child + index + 1,
                sizeof(*parent->child) * (parent->size - index));
    }
    if (--parent->size == 0)
    {
        free(parent->child);
        parent->child = NULL;
    }
    child->packed = 0;
    return child;
}

/* Pop node matching 'key' */
json_t *json_object_pop(json_t *parent, const char *key)
{
    unsigned index = json_index(parent, key);

    if (index == JSON_NOT_FOUND)
    {
        return NULL;
    }
    return pop(parent, index);
}

/* Pop node from 'parent' at position 'index' if parent is an array */
json_t *json_array_pop(json_t *parent, size_t index)
{
    if ((parent == NULL) || (parent->type != JSON_ARRAY) || (parent->size == 0))
    {
        return NULL;
    }
    return pop(parent, (unsigned)index);
}

/* Pop node from 'parent' at position 'index' */
json_t *json_pop_at(json_t *parent, size_t index)
{
    if ((parent == NULL) || (parent->size == 0))
    {
        return NULL;
    }
    return pop(parent, (unsigned)index);
}

/* Move node from 'target' to 'source' on distinct iterables */
static json_t *move(json_t *source, unsigned source_index,
    json_t *target, unsigned index)
{
    if (index > target->size)
    {
        index = target->size;
    }
 
    unsigned size = next_size(target->size);

    if (size > target->size) 
    {
        json_t **temp = realloc(target->child, sizeof(*temp) * size);

        if (temp == NULL)
        {
            return NULL;
        }
        target->child = temp;
    }
    if (index < target->size)
    {
        memmove(target->child + index + 1,
                target->child + index,
                sizeof(*target->child) * (target->size - index));
    }

    json_t *child = pop(source, source_index);

    if (target->type != source->type)
    {
        free(child->key);
        child->key = NULL;
    }
    child->packed = 1;
    target->child[index] = child;
    target->size++;
    return child;    
}

/* Move node from 'target' to 'source' on the same iterable */
static json_t *move_from_to(json_t *parent, unsigned a, unsigned b)
{
    if (a >= parent->size)
    {
        a = parent->size - 1;
    }
    if (b >= parent->size)
    {
        b = parent->size - 1;
    }

    json_t *temp = parent->child[a];

    if (a > b)
    {
        memmove(parent->child + b + 1,
                parent->child + b,
                sizeof(*parent->child) * (a - b));
    }
    else if (b > a)
    {
        memmove(parent->child + a,
                parent->child + a + 1,
                sizeof(*parent->child) * (b - a));
    }
    parent->child[b] = temp;
    return temp;
}

/* Move node from 'target' to 'source' */
json_t *json_move(json_t *source, size_t a, json_t *target, size_t b)
{
    if ((source == NULL) || (source->size == 0) || (target == NULL))
    {
        return NULL;
    }
    if ((target->type == JSON_ARRAY)
    || ((target->type == JSON_OBJECT) && (source->type == JSON_OBJECT)))
    {
        if (((a == JSON_TAIL) || (a < source->size)) &&
            ((b == JSON_TAIL) || (b <= target->size)))
        {
            if (source != target)
            {
                return move(source, (unsigned)a, target, (unsigned)b);
            }
            else
            {
                return move_from_to(target, (unsigned)a, (unsigned)b);
            }
        }
    }
    return NULL;
}

/**
 * Swap node at position 'a' in 'source' with node at position 'b' in 'target'
 * Parents must have the same type
 */
json_t *json_swap(json_t *source, size_t a, json_t *target, size_t b)
{
    if ((source == NULL) || (target == NULL) || (source->type != target->type))
    {
        return NULL;
    }
    if ((a == JSON_TAIL) && (source->size > 0))
    {
        a = source->size - 1;
    }
    if ((b == JSON_TAIL) && (target->size > 0))
    {
        b = target->size - 1;
    }
    if ((a < source->size) && (b < target->size)) 
    {
        json_t *temp = source->child[a];

        source->child[a] = target->child[b];
        target->child[b] = temp;
        return temp;
    }
    return NULL;
}

/* Deletes the first node matching 'key' */
int json_object_delete(json_t *parent, const char *key)
{
    return json_delete(json_object_pop(parent, key));
}

/* Deletes the node at position 'index' if 'parent' is an array */
int json_array_delete(json_t *parent, size_t index)
{
    return json_delete(json_array_pop(parent, index));
}

/* Deletes the node at position 'index' */
int json_delete_at(json_t *parent, size_t index)
{
    return json_delete(json_pop_at(parent, index));
}

/* json_delete helper */
static void delete_node(json_t *node)
{
    if (node->type == JSON_STRING)
    {
        free(node->string);
    }
    else if (node->size > 0)
    {
        free(node->child);
    }
    free(node->key);
    free(node);
}

/* json_delete recursive helper */
static void delete_tree(json_t *node)
{
    for (unsigned i = 0; i < node->size; i++)
    {
        if (node->child[i]->size > 0)
        {
            delete_tree(node->child[i]);
        }
        else
        {
            delete_node(node->child[i]);
        }
    }
    delete_node(node);
}

/* Deletes internal nodes */
int json_delete_children(json_t *node)
{
    for (unsigned i = 0; i < node->size; i++)
    {
        if (node->child[i]->size > 0)
        {
            delete_tree(node->child[i]);
        }
        else
        {
            delete_node(node->child[i]);
        }
    }
    if (node->size > 0)
    {
        node->child = NULL;
        node->size = 0;
        return 1;
    }
    return 0;
}

/**
 * Deletes a non internal node
 * Returns 1 if 'node' can be deleted, 0 otherwise
 */
int json_delete(json_t *node)
{
    if (!node || node->packed)
    {
        return 0;
    }
    delete_tree(node);
    return 1;
}

/**
 * Deletes a non internal node
 * Useful for destructors with a callback expecting a 'void *' as param
 */  
void json_free(void *node)
{
    if (!node || ((json_t *)node)->packed)
    {
        return;
    }
    delete_tree(node);
}

