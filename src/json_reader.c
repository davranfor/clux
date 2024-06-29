/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "json_private.h"
#include "json_unicode.h"

static const char *type_name[] =
{
    "Undefined",
    "Object",
    "Array",
    "String",
    "Integer",
    "Real",
    "Boolean",
    "Null",
};

enum json_type json_type(const json *node)
{
    if (node == NULL)
    {
        return JSON_UNDEFINED;
    }
    return node->type;
}

const char *json_type_name(const json *node)
{
    return type_name[json_type(node)];
}

/**
 * json_key():  returns NULL if the node doesn't have a key
 * json_name(): returns  ""  if the node doesn't have a key
 */

const char *json_key(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->name;
}

const char *json_name(const json *node)
{
    if ((node == NULL) || (node->name == NULL))
    {
        return "";
    }
    return node->name;
}

const char *json_string(const json *node)
{
    if ((node == NULL) || (node->type != JSON_STRING))
    {
        return NULL;
    }
    return node->value.string;
}

double json_number(const json *node)
{
    if ((node == NULL) || (node->type == JSON_STRING))
    {
        return 0.0;
    }
    return node->value.number;
}

int json_boolean(const json *node)
{
    if ((node == NULL) || (node->type == JSON_STRING))
    {
        return 0;
    }
    return node->value.number != 0;
}

int json_is_any(const json *node)
{
    return node != NULL;
}

int json_is_iterable(const json *node)
{
    return (node != NULL)
        && ((node->type == JSON_OBJECT) || (node->type == JSON_ARRAY));
}

int json_is_scalar(const json *node)
{
    return (node != NULL)
        && ((node->type != JSON_OBJECT) && (node->type != JSON_ARRAY));
}

int json_is_object(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_OBJECT);
}

int json_is_array(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_ARRAY);
}

int json_is_string(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_STRING);
}

int json_is_integer(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_INTEGER);
}

int json_is_unsigned(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_INTEGER)
        && (node->value.number >= 0);
}

int json_is_real(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_REAL);
}

int json_is_number(const json *node)
{
    return (node != NULL)
        && ((node->type == JSON_INTEGER) || (node->type == JSON_REAL));
}

int json_is_boolean(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_BOOLEAN);
}

int json_is_true(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_BOOLEAN)
        && (node->value.number != 0);
}

int json_is_false(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_BOOLEAN)
        && (node->value.number == 0);
}

int json_is_null(const json *node)
{
    return (node != NULL)
        && (node->type == JSON_NULL);
}

/**
 * Useful to return a non constant 'json *node' when a function
 * gets 'const json *node' as argument and returns the same node.
 * For example calling 'x = json_pointer(node, "");'
 * returns the same node that was passed.
 * Use it with care.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
json *json_self(const json *node)
{
    return (json *)node;
}

json *json_root(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    while (node->parent != NULL)
    {
        node = node->parent;
    }
    return (json *)node;
}
#pragma GCC diagnostic pop

json *json_parent(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->parent;
}

json *json_child(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->head;
}

json *json_head(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->head;
}

json *json_prev(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->prev;
}

json *json_next(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->next;
}

json *json_tail(const json *node)
{
    if (node == NULL)
    {
        return NULL;
    }
    return node->tail;
}

/* Locates a child by index */
json *json_at(const json *root, size_t index)
{
    if ((root == NULL) || (index >= root->size))
    {
        return NULL;
    }

    json *node;

    if (root->size - index > index)
    {
        node = root->head;
        for (size_t iter = 0; iter < index; iter++)
        {
            node = node->next;
        }
    }
    else
    {
        node = root->tail;
        for (size_t iter = root->size - 1; iter > index; iter--)
        {
            node = node->prev;
        }
    }
    return node;
}

/* Locates a child by name */
json *json_find(const json *root, const char *name)
{
    if ((root == NULL) || (root->type != JSON_OBJECT) || (name == NULL))
    {
        return NULL;
    }
    for (json *node = root->head; node != NULL; node = node->next)
    {
        assert(node->name != NULL);
        if (strcmp(node->name, name) == 0)
        {
            return node;
        }
    }
    return NULL;
}

/* Locates a previous sibling by name */
json *json_find_prev(const json *root, const char *name)
{
    if ((root == NULL) || (root->name == NULL) || (name == NULL))
    {
        return NULL;
    }
    for (json *node = root->prev; node != NULL; node = node->prev)
    {
        assert(node->name != NULL);
        if (strcmp(node->name, name) == 0)
        {
            return node;
        }
    }
    return NULL;
}

/* Locates a next sibling by name */
json *json_find_next(const json *root, const char *name)
{
    if ((root == NULL) || (root->name == NULL) || (name == NULL))
    {
        return NULL;
    }
    for (json *node = root->next; node != NULL; node = node->next)
    {
        assert(node->name != NULL);
        if (strcmp(node->name, name) == 0)
        {
            return node;
        }
    }
    return NULL;
}

/* Locates a child by node */
json *json_locate(const json *root, const json *node)
{
    if ((root == NULL) || (node == NULL))
    {
        return NULL;
    }
    for (json *item = root->head; item != NULL; item = item->next)
    {
        if (json_equal(item, node))
        {
            return item;
        }
    }
    return NULL;
}

/* Locates a previous sibling by node */
json *json_locate_prev(const json *root, const json *node)
{
    if ((root == NULL) || (node == NULL))
    {
        return NULL;
    }
    for (json *item = root->prev; item != NULL; item = item->prev)
    {
        if (json_equal(item, node))
        {
            return item;
        }
    }
    return NULL;
}

/* Locates a next sibling by node */
json *json_locate_next(const json *root, const json *node)
{
    if ((root == NULL) || (node == NULL))
    {
        return NULL;
    }
    for (json *item = root->next; item != NULL; item = item->next)
    {
        if (json_equal(item, node))
        {
            return item;
        }
    }
    return NULL;
}

/* Length of an UTF8 string */
size_t json_length(const json *node)
{
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return utf8_length(node->value.string);
    }
    return 0;
}

/* Number of childs of an iterable */
size_t json_size(const json *node)
{
    if (node == NULL)
    {
        return 0;
    }
    return node->size;
}

/* Position of the node into an interable */
size_t json_offset(const json *node)
{
    if (node == NULL)
    {
        return 0;
    }

    size_t offset = 0;

    for (node = node->prev; node != NULL; node = node->prev)
    {
        offset++;
    }
    return offset;
}

/* json_height helper */
static int tree_height(const json *node, int depth, void *height)
{
    (void)node;
    if (depth > *(int *)height)
    {
        *(int *)height = depth;
    }
    return 1;
}

/* Number of edges from the leaf node to the passed node */
size_t json_height(const json *node)
{
    int height = 0;

    json_walk(node, tree_height, &height);
    return (size_t)height;
}

/* Number of edges from the root node to the passed node */
size_t json_depth(const json *node)
{
    if (node == NULL)
    {
        return 0;
    }

    size_t depth = 0;

    for (node = node->parent; node != NULL; node = node->parent)
    {
        depth++;
    }
    return depth;
}

/**
 * Compares two nodes by value
 * Returns
 *  < 0 if a < b
 *  > 0 if a > b
 *  0 otherwise
 */
int json_compare(const json *a, const json *b)
{
    if ((a == NULL) || (b == NULL))
    {
        return a ? +1 : b ? -1 : 0;
    }
    if (a->type != b->type)
    {
        return a->type > b->type ? +1 : -1;
    }
    if (a->type != JSON_STRING)
    {
        return
            a->value.number < b->value.number ? -1 :
            a->value.number > b->value.number;
    }
    return strcmp(a->value.string, b->value.string);
}

/* json_equal helper */
static int equal(const json *a, const json *b, int depth)
{
    if (depth > 0)
    {
        if ((a->name == NULL) != (b->name == NULL))
        {
            return 0;
        }
        if ((a->name != NULL) && strcmp(a->name, b->name))
        {
            return 0;
        }
    }
    if ((a->type != b->type) || (a->size != b->size))
    {
        return 0;
    }
    return a->type != JSON_STRING
         ? a->value.number == b->value.number
         : strcmp(a->value.string, b->value.string) == 0;
}

/**
 * Compares two nodes
 * Returns 1 when nodes are equal, 0 otherwise
 */
int json_equal(const json *a, const json *b)
{
    int depth = 0, flag = 1;

    while ((a != NULL) && (b != NULL))
    {
        if ((flag == 1) && !equal(a, b, depth))
        {
           return 0;
        }
        if ((flag == 1) && (a->head != NULL))
        {
            a = a->head;
            b = b->head;
            depth++;
        }
        else if ((depth > 0) && (a->next != NULL))
        {
            a = a->next;
            b = b->next;
            flag = 1;
        }
        else if (depth-- > 0)
        {
            a = a->parent;
            b = b->parent;
            flag = 0;
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

/**
 * Sends all nodes to a callback func providing depth and user-data
 * Exit when all nodes are read or callback returns 0 (example in json_height())
 */
int json_walk(const json *node, json_walk_callback callback, void *data)
{
    int depth = 0, flag = 1;

    while (node != NULL)
    {
        if ((flag == 1) && (callback(node, depth, data) == 0))
        {
            return 0;
        }
        if ((flag == 1) && (node->head != NULL))
        {
            node = node->head;
            depth++;
        }
        else if ((depth > 0) && (node->next != NULL))
        {
            node = node->next;
            flag = 1;
        }
        else if (depth-- > 0)
        {
            node = node->parent;
            flag = 0;
        }
        else
        {
            break;
        }
    }
    return 1;
}

