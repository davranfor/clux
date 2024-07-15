/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clib_string.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_writer.h"

static json *new_string(char *value)
{
    if (value == NULL)
    {
        return NULL;
    }

    json *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_STRING;
        node->value.string = value;
    }
    else
    {
        free(value);
    }
    return node;
}

static json *new_number(enum json_type type, double value)
{
    json *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = type;
        node->value.number = value;
    }
    return node;
}

json *json_new_object(void)
{
    return new_number(JSON_OBJECT, 0);
}

json *json_new_array(void)
{
    return new_number(JSON_ARRAY, 0);
}

json *json_new_format(const char *fmt, ...)
{
    if (fmt == NULL)
    {
        return NULL;
    }

    va_list args;

    va_start(args, fmt);

    char *str = string_vprint(fmt, args);

    va_end(args);
    return new_string(str);
}

json *json_new_string(const char *value)
{
    if (value == NULL)
    {
        return NULL;
    }
    return new_string(string_clone(value));
}

json *json_new_integer(double value)
{
    return new_number(JSON_INTEGER, trunc(value));
}

json *json_new_real(double value)
{
    return new_number(JSON_REAL, value);
}

json *json_new_boolean(int value)
{
    return new_number(JSON_BOOLEAN, value ? 1 : 0);
}

json *json_new_null(void)
{
    return new_number(JSON_NULL, 0);
}

static json *let_string(const char *key, char *value)
{
    if (value == NULL)
    {
        return NULL;
    }

    char *name = string_clone(key);

    if (name == NULL)
    {
        free(value);
        return NULL;
    }

    json *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = JSON_STRING;
        node->name = name;
        node->value.string = value;
    }
    else
    {
        free(name);
        free(value);
    }
    return node;
}

static json *let_number(enum json_type type, const char *key, double value)
{
    char *name = string_clone(key);

    if (name == NULL)
    {
        return NULL;
    }

    json *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = type;
        node->name = name;
        node->value.number = value;
    }
    else
    {
        free(name);
    }
    return node;
}

json *json_new_named_object(const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }
    return let_number(JSON_OBJECT, name, 0);
}

json *json_new_named_array(const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }
    return let_number(JSON_ARRAY, name, 0);
}

json *json_new_named_format(const char *name, const char *fmt, ...)
{
    if ((name == NULL) || (fmt == NULL))
    {
        return NULL;
    }

    va_list args;

    va_start(args, fmt);

    char *str = string_vprint(fmt, args);

    va_end(args);
    return let_string(name, str);
}

json *json_new_named_string(const char *name, const char *value)
{
    if ((name == NULL) || (value == NULL))
    {
        return NULL;
    }
    return let_string(name, string_clone(value));
}

json *json_new_named_integer(const char *name, double value)
{
    if (name == NULL)
    {
        return NULL;
    }
    return let_number(JSON_INTEGER, name, trunc(value));
}

json *json_new_named_real(const char *name, double value)
{
    if (name == NULL)
    {
        return NULL;
    }
    return let_number(JSON_REAL, name, value);
}

json *json_new_named_boolean(const char *name, int value)
{
    if (name == NULL)
    {
        return NULL;
    }
    return let_number(JSON_BOOLEAN, name, value ? 1 : 0);
}

json *json_new_named_null(const char *name)
{
    if (name == NULL)
    {
        return NULL;
    }
    return let_number(JSON_NULL, name, 0);
}

json *json_set_name(json *node, const char *name)
{
    if ((node == NULL) || ((node->parent != NULL) && (!node->name != !name)))
    {
        return NULL;
    }

    char *str = NULL;

    if ((name != NULL) && !(str = string_clone(name)))
    {
        return NULL;
    }
    free(node->name);
    node->name = str;
    return node;
}

/* set helpers */

static void set_empty(json *node)
{
    switch (node->type)
    {
        case JSON_OBJECT:
        case JSON_ARRAY:
            while (json_delete(node->head));
            return;
        case JSON_STRING:
            free(node->value.string);
            return;
        default:
            return;
    }
}

static json *set_string(json *node, char *value)
{
    if (value == NULL)
    {
        return NULL;
    }
    set_empty(node);
    node->type = JSON_STRING;
    node->value.string = value;
    return node;
}

static json *set_number(json *node, enum json_type type, double value)
{
    set_empty(node);
    node->type = type;
    node->value.number = value;
    return node;
}

json *json_set_object(json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return set_number(node, JSON_OBJECT, 0);
}

json *json_set_array(json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return set_number(node, JSON_ARRAY, 0);
}

json *json_set_format(json *node, const char *fmt, ...)
{
    if ((node == NULL) || (fmt == NULL))
    {
        return NULL;
    }

    va_list args;

    va_start(args, fmt);

    char *str = string_vprint(fmt, args);

    va_end(args);
    return set_string(node, str);
}

json *json_set_string(json *node, const char *value)
{
    if ((node == NULL) || (value == NULL))
    {
        return NULL;
    }
    return set_string(node, string_clone(value));
}

json *json_set_integer(json *node, double value)
{
    if (node == NULL)
    {
        return NULL;
    }
    return set_number(node, JSON_INTEGER, trunc(value));
}

json *json_set_real(json *node, double value)
{
    if (node == NULL)
    {
        return NULL;
    }
    return set_number(node, JSON_REAL, value);
}

json *json_set_boolean(json *node, int value)
{
    if (node == NULL)
    {
        return NULL;
    }
    return set_number(node, JSON_BOOLEAN, value ? 1 : 0);
}

json *json_set_null(json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return set_number(node, JSON_NULL, 0);
}

json *json_let_object(json *parent, const char *name)
{
    if (!json_is_object(parent) || (name == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_number(JSON_OBJECT, name, 0))
        : set_number(child, JSON_OBJECT, 0);
}

json *json_let_array(json *parent, const char *name)
{
    if (!json_is_object(parent) || (name == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_number(JSON_ARRAY, name, 0))
        : set_number(child, JSON_ARRAY, 0);
}

json *json_let_format(json *parent, const char *name, const char *fmt, ...)
{
    if (!json_is_object(parent) || (name == NULL) || (fmt == NULL))
    {
        return NULL;
    }

    va_list args;

    va_start(args, fmt);

    char *str = string_vprint(fmt, args);

    va_end(args);

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_string(name, str))
        : set_string(child, str);
}

json *json_let_string(json *parent, const char *name, const char *value)
{
    if (!json_is_object(parent) || (name == NULL) || (value == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_string(name, string_clone(value)))
        : set_string(child, string_clone(value));
}

json *json_let_integer(json *parent, const char *name, double value)
{
    if (!json_is_object(parent) || (name == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_number(JSON_INTEGER, name, trunc(value)))
        : set_number(child, JSON_INTEGER, trunc(value));
}

json *json_let_real(json *parent, const char *name, double value)
{
    if (!json_is_object(parent) || (name == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_number(JSON_REAL, name, value))
        : set_number(child, JSON_REAL, value);
}

json *json_let_boolean(json *parent, const char *name, int value)
{
    if (!json_is_object(parent) || (name == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_number(JSON_BOOLEAN, name, value ? 1 : 0))
        : set_number(child, JSON_BOOLEAN, value ? 1 : 0);
}

json *json_let_null(json *parent, const char *name)
{
    if (!json_is_object(parent) || (name == NULL))
    {
        return NULL;
    }

    json *child = json_find(parent, name);

    return (child == NULL)
        ? json_push_back(parent, let_number(JSON_NULL, name, 0))
        : set_number(child, JSON_NULL, 0);
}

/* push helper */
static int not_pushable(const json *parent, const json *child)
{
    if (!json_is_iterable(parent) || (child == NULL) || (child->parent != NULL))
    {
        return 1;
    }
    // parent being object and child without name
    // or
    // parent being array and child with name 
    return (parent->type == JSON_OBJECT) ^ (child->name != NULL);
}

json *json_push_front(json *parent, json *child)
{
    if (not_pushable(parent, child))
    {
        return NULL;
    }
    if (parent->head == NULL)
    {
        parent->tail = child;
    }
    else
    {
        child->next = parent->head;
        parent->head->prev = child;
    }
    child->parent = parent;
    parent->head = child;
    parent->size++;
    return child;
}

json *json_push_back(json *parent, json *child)
{
    if (not_pushable(parent, child))
    {
        return NULL;
    }
    if (parent->head == NULL)
    {
        parent->head = child;
    }
    else
    {
        child->prev = parent->tail;
        parent->tail->next = child;
    }
    child->parent = parent;
    parent->tail = child;
    parent->size++;
    return child;
}

json *json_push_before(json *where, json *child)
{
    json *parent = json_parent(where);

    if (not_pushable(parent, child))
    {
        return NULL;
    }
    if (parent->head == where)
    {
        parent->head = child;
    }
    else
    {
        child->prev = where->prev;
        where->prev->next = child;
    }
    child->parent = parent;
    child->next = where;
    where->prev = child;
    parent->size++;
    return child;
}

json *json_push_after(json *where, json *child)
{
    json *parent = json_parent(where);

    if (not_pushable(parent, child))
    {
        return NULL;
    }
    if (parent->tail == where)
    {
        parent->tail = child;
    }
    else
    {
        child->next = where->next;
        where->next->prev = child;
    }
    child->parent = parent;
    child->prev = where;
    where->next = child;
    parent->size++;
    return child;
}

json *json_push_at(json *parent, json *child, size_t index)
{
    if (not_pushable(parent, child))
    {
        return NULL;
    }
    if (parent->head == NULL)
    {
        parent->head = child;
        parent->tail = child;
    }
    else
    {
        json *node = json_at(parent, index);

        if (node == NULL)
        {
            child->prev = parent->tail;
            parent->tail->next = child;
            parent->tail = child;
        }
        else
        {
            if (parent->head == node)
            {
                parent->head = child;
            }
            else
            {
                child->prev = node->prev;
                node->prev->next = child;
            }
            child->next = node;
            node->prev = child;
        }
    }
    child->parent = parent;
    parent->size++;
    return child;
}

json *json_pop(json *child)
{
    json *parent = json_parent(child);

    if (parent == NULL)
    {
        return NULL;
    }
    if (parent->head == child)
    {
        parent->head = child->next;
    }
    else
    {
        child->prev->next = child->next;
    }
    if (parent->tail == child)
    {
        parent->tail = child->prev; 
    }
    else
    {
        child->next->prev = child->prev;
    }
    child->parent = NULL;
    child->prev = NULL;
    child->next = NULL; 
    parent->size--;
    return child;
}

json *json_pop_front(json *parent)
{
    json *child = json_head(parent);

    if (child == NULL)
    {
        return NULL;
    }
    parent->head = child->next;
    if (parent->tail == child)
    {
        parent->tail = NULL;
    }
    else
    {
        child->next->prev = NULL;
        child->next = NULL;
    }
    child->parent = NULL;
    parent->size--;
    return child;
}

json *json_pop_back(json *parent)
{
    json *child = json_tail(parent);

    if (child == NULL)
    {
        return NULL;
    }
    parent->tail = child->prev;
    if (parent->head == child)
    {
        parent->head = NULL;
    }
    else
    {
        child->prev->next = NULL;
        child->prev = NULL;
    }
    child->parent = NULL;
    parent->size--;
    return child;
}

json *json_pop_at(json *parent, size_t index)
{
    json *child = json_at(parent, index);

    if (child == NULL)
    {
        return NULL;
    }
    if (parent->head == child)
    {
        parent->head = child->next;
    }
    else 
    {
        child->prev->next = child->next;
    }
    if (parent->tail == child)
    {
        parent->tail = child->prev;
    }
    else
    {
        child->next->prev = child->prev;
    }
    child->parent = NULL;
    child->prev = NULL;
    child->next = NULL;
    parent->size--;
    return child;
}

json *json_delete(json *node)
{
    json *parent = json_parent(node);
    json *next = NULL;

    if (parent != NULL)
    {
        if (parent->head == node)
        {
            parent->head = node->next;
        }
        else
        {
            node->prev->next = node->next;
        }
        if (parent->tail == node)
        {
            parent->tail = node->prev;
        }
        else
        {
            node->next->prev = node->prev;
            next = node->next;
        }
        node->parent = NULL;
        node->prev = NULL;
        node->next = NULL;
        parent->size--;
    }
    json_free(node);
    return next;
}

void json_free(json *node)
{
    const json *parent = node ? node->parent : NULL;

    while (node != parent)
    {
        json *next = node->head;

        node->head = NULL;
        if (next == NULL)
        {
            if (node->next != NULL)
            {
                next = node->next;
            }
            else
            {
                next = node->parent;
            }
            free(node->name);
            if (node->type == JSON_STRING)
            {
                free(node->value.string);
            }
            free(node);
        }
        node = next;
    }
}

