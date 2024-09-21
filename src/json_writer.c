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

json_t *json_new_object(void)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_OBJECT;
    }
    return node;
}

json_t *json_new_array(void)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_ARRAY;
    }
    return node;
}

json_t *json_new_string(const char *text)
{
    if (text == NULL)
    {
        return NULL;
    }

    char *string = string_clone(text);

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

json_t *json_new_integer(double number)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = is_safe_integer(number)
            ? JSON_INTEGER
            : JSON_REAL;
        node->number = trunc(number);
    }
    return node;
}

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

json_t *json_new_null(void)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_NULL;
    }
    return node;
}

static json_t *push(json_t *parent, unsigned index, const char *name, json_t *child)
{
    if ((child == NULL) || (parent == child) || child->packed)
    {
        return NULL;
    }
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

    unsigned size = next_size(parent->size);

    if (size > parent->size) 
    {
        json_t **childs = realloc(parent->child, sizeof(*childs) * size);

        if (childs == NULL)
        {
            free(key);
            return NULL;
        }
        parent->child = childs;
    }
    if (index < parent->size)
    {
        memmove(parent->child + index + 1,
                parent->child + index,
                sizeof(*parent->child) * (parent->size - index));
    }
    child->key = key;
    child->packed = 1;
    parent->child[index] = child;
    parent->size++;
    return child;    
}

json_t *json_object_push(json_t *parent, size_t index, const char *key, json_t *child)
{
    if ((parent == NULL) || (parent->type != JSON_OBJECT) || (key == NULL))
    {
        return NULL;
    }
    return push(parent, (unsigned)index, key, child);
}

json_t *json_array_push(json_t *parent, size_t index, json_t *child)
{
    if ((parent == NULL) || (parent->type != JSON_ARRAY))
    {
        return NULL;
    }
    return push(parent, (unsigned)index, NULL, child);
}

/**
 * Not exported in json_writer.h
 * Declared as 'extern' in json_parser.c
 */
json_t *json_parser_push(json_t *, json_t *);

json_t *json_parser_push(json_t *parent, json_t *child)
{
    return push(parent, (unsigned)-1, NULL, child);
}

static json_t *pop(json_t *parent, unsigned index)
{
    if (parent->size == 0)
    {
        return NULL;
    }
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
    }
    free(child->key);
    child->key = NULL;
    child->packed = 0;
    return child;
}

json_t *json_pop_by_key(json_t *parent, const char *key)
{
    unsigned index = json_index(parent, key);

    if (index == -1u)
    {
        return NULL;
    }
    return pop(parent, (unsigned)index);
}

json_t *json_pop_by_index(json_t *parent, size_t index)
{
    if (parent == NULL)
    {
        return NULL;
    }
    return pop(parent, (unsigned)index);
}

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

int json_delete(json_t *node)
{
    if (node == NULL)
    {
        return 0;
    }
    if (node->packed)
    {
        return -1;
    }
    delete_tree(node);
    return 1;
}

int json_delete_by_key(json_t *parent, const char *key)
{
    return json_delete(json_pop_by_key(parent, key));
}

int json_delete_by_index(json_t *parent, size_t index)
{
    return json_delete(json_pop_by_index(parent, index));
}

/*
static int json_compare_key(const void *pa, const void *pb)
{
    const json_t *a = *(json_t * const *)pa;
    const json_t *b = *(json_t * const *)pb;

    return strcmp(a->key, b->key);
}

static void json_sort(json_t *node, int (*callback)(const void *, const void *))
{
    if ((node != NULL) && (node->size > 1))
    {
        qsort(node->child, node->size, sizeof *node->child, callback);
    }
}
*/

