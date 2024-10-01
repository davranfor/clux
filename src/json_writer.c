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

static json_t *push(json_t *parent, unsigned index, const char *name,
    json_t *child)
{
    if ((parent == child) || child->packed)
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

static json_t *pop(json_t *parent, unsigned index)
{
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

json_t *json_pop_by_key(json_t *parent, const char *key)
{
    unsigned index = json_index(parent, key);

    if (index == JSON_NOT_FOUND)
    {
        return NULL;
    }
    return pop(parent, index);
}

json_t *json_pop_by_index(json_t *parent, size_t index)
{
    if ((parent == NULL) || (parent->size == 0))
    {
        return NULL;
    }
    return pop(parent, (unsigned)index);
}

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
        json_t **childs = realloc(target->child, sizeof(*childs) * size);

        if (childs == NULL)
        {
            return NULL;
        }
        target->child = childs;
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

json_t *json_move_child(json_t *source, size_t a, json_t *target, size_t b)
{
    if ((source == NULL) || (source->size == 0) ||
        (target == NULL) || (target->size == 0))
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

json_t *json_swap_child(json_t *source, size_t a, json_t *target, size_t b)
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

int json_delete_root(json_t *node)
{
    if (!node || node->packed)
    {
        return 0;
    }
    delete_tree(node);
    return 1;
}

int json_delete_by_key(json_t *parent, const char *key)
{
    return json_delete_root(json_pop_by_key(parent, key));
}

int json_delete_by_index(json_t *parent, size_t index)
{
    return json_delete_root(json_pop_by_index(parent, index));
}

void json_free(json_t *node)
{
    if (!node || node->packed)
    {
        return;
    }
    delete_tree(node);
}

