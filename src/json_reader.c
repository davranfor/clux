/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include <assert.h>
#include <math.h>
#include "clib_string.h"
#include "json_private.h"
#include "json_reader.h"

static const char *type_name[] =
{
    "undefined",
    "object",
    "array",
    "string",
    "integer",
    "real",
    "boolean",
    "null",
};

enum json_type json_type(const json *node)
{
    return node != NULL ? node->type : JSON_UNDEFINED;
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
    return node != NULL ? node->name : NULL;
}

const char *json_name(const json *node)
{
    if ((node != NULL) && (node->name != NULL))
    {
        return node->name;
    }
    return "";
}

/**
 * json_text():   returns  ""  if the node is not a string
 * json_string(): returns NULL if the node is not a string
 */

const char *json_text(const json *node)
{
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return node->value.string;
    }
    return "";
}

const char *json_string(const json *node)
{
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return node->value.string;
    }
    return NULL;
}

double json_number(const json *node)
{
    if ((node != NULL) && (node->type != JSON_STRING))
    {
        return node->value.number;
    }
    return 0.0;
}

int json_boolean(const json *node)
{
    if ((node != NULL) && (node->type != JSON_STRING))
    {
        return node->value.number != 0;
    }
    return 0;
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
 * gets 'const json *' as argument and returns the same node.
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
    return node != NULL ? node->parent : NULL;
}

json *json_child(const json *node)
{
    return node != NULL ? node->head : NULL;
}

json *json_head(const json *node)
{
    return node != NULL ? node->head : NULL;
}

json *json_prev(const json *node)
{
    return node != NULL ? node->prev : NULL;
}

json *json_next(const json *node)
{
    return node != NULL ? node->next : NULL;
}

json *json_tail(const json *node)
{
    return node != NULL ? node->tail : NULL;
}

json *json_object_head(const json *node)
{
    if ((node != NULL) && (node->type == JSON_OBJECT))
    {
        return node->head;
    }
    return NULL;
}

json *json_object_tail(const json *node)
{
    if ((node != NULL) && (node->type == JSON_OBJECT))
    {
        return node->tail;
    }
    return NULL;
}

json *json_array_head(const json *node)
{
    if ((node != NULL) && (node->type == JSON_ARRAY))
    {
        return node->head;
    }
    return NULL;
}

json *json_array_tail(const json *node)
{
    if ((node != NULL) && (node->type == JSON_ARRAY))
    {
        return node->tail;
    }
    return NULL;
}

/* Locates a child by index */
json *json_at(const json *parent, size_t index)
{
    if ((parent == NULL) || (index >= parent->size))
    {
        return NULL;
    }

    json *node;

    if (parent->size - index > index)
    {
        node = parent->head;
        for (size_t iter = 0; iter < index; iter++)
        {
            node = node->next;
        }
    }
    else
    {
        node = parent->tail;
        for (size_t iter = parent->size - 1; iter > index; iter--)
        {
            node = node->prev;
        }
    }
    return node;
}

/* Locates a child by name */
json *json_find(const json *parent, const char *name)
{
    if ((parent == NULL) || (parent->type != JSON_OBJECT) || (name == NULL))
    {
        return NULL;
    }
    for (json *node = parent->head; node != NULL; node = node->next)
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
json *json_find_prev(const json *parent, const char *name)
{
    if ((parent == NULL) || (parent->name == NULL) || (name == NULL))
    {
        return NULL;
    }
    for (json *node = parent->prev; node != NULL; node = node->prev)
    {
        if (strcmp(node->name, name) == 0)
        {
            return node;
        }
    }
    return NULL;
}

/* Locates a next sibling by name */
json *json_find_next(const json *parent, const char *name)
{
    if ((parent == NULL) || (parent->name == NULL) || (name == NULL))
    {
        return NULL;
    }
    for (json *node = parent->next; node != NULL; node = node->next)
    {
        if (strcmp(node->name, name) == 0)
        {
            return node;
        }
    }
    return NULL;
}

/* Locates a child by node */
json *json_locate(const json *parent, const json *what)
{
    if ((parent == NULL) || (what == NULL))
    {
        return NULL;
    }
    for (json *item = parent->head; item != NULL; item = item->next)
    {
        if (json_equal(item, what))
        {
            return item;
        }
    }
    return NULL;
}

/* Locates a previous sibling by node */
json *json_locate_prev(const json *node, const json *what)
{
    if ((node == NULL) || (what == NULL))
    {
        return NULL;
    }
    for (json *item = node->prev; item != NULL; item = item->prev)
    {
        if (json_equal(item, what))
        {
            return item;
        }
    }
    return NULL;
}

/* Locates a next sibling by node */
json *json_locate_next(const json *node, const json *what)
{
    if ((node == NULL) || (what == NULL))
    {
        return NULL;
    }
    for (json *item = node->next; item != NULL; item = item->next)
    {
        if (json_equal(item, what))
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
    return node != NULL ? node->size : 0;
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
static int tree_height(const json *node, size_t depth, void *height)
{
    (void)node;
    if (depth > *(size_t *)height)
    {
        *(size_t *)height = depth;
    }
    return 1;
}

/* Number of edges from the leaf node to the passed node */
size_t json_height(const json *node)
{
    size_t height = 0;

    json_walk(node, tree_height, &height);
    return height;
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
    if ((a->type != b->type) || (a->size != b->size))
    {
        return 0;
    }
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
    int depth = 0;
    int flag = 1;

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
        else if (depth > 0)
        {
            if (a->next != NULL)
            {
                a = a->next;
                b = b->next;
                flag = 1;
            }
            else
            {
                a = a->parent;
                b = b->parent;
                flag = 0;
                depth--;
            }
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

/**
 * Sends all nodes to a callback func providing node, depth and user-data
 * Exit when all nodes are read or callback returns <=0 (example in json_height())
 */
int json_walk(const json *node, json_walk_callback callback, void *data)
{
    size_t depth = 0;
    int flag = 1;

    while (node != NULL)
    {
        if (flag == 1)
        {
            int rc = callback(node, depth, data);
            
            if (rc <= 0)
            {
                return rc;
            }
        }
        if ((flag == 1) && (node->head != NULL))
        {
            node = node->head;
            depth++;
        }
        else if (depth > 0)
        {
            if (node->next != NULL)
            {
                node = node->next;
                flag = 1;
            }
            else
            {
                node = node->parent;
                flag = 0;
                depth--;
            }
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

