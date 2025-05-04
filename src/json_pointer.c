/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "clib_check.h"
#include "json_private.h"
#include "json_pointer.h"

/*
---------------------------------------------------------------------
JavaScript Object Notation (JSON) Pointer
---------------------------------------------------------------------
Evaluation of a JSON Pointer begins with a reference to the root
value of a JSON document and completes with a reference to some value
within the document. Each reference token in the JSON Pointer is
evaluated sequentially.

Evaluation of each reference token begins by decoding any escaped
character sequence. This is performed by first transforming any
occurrence of the sequence '~1' to '/', and then transforming any
occurrence of the sequence '~0' to '~'. By performing the
substitutions in this order, an implementation avoids the error of
turning '~01' first into '~1' and then into '/', which would be
incorrect (the string '~01' correctly becomes '~1' after
transformation).

The reference token then modifies which value is referenced according
to the following scheme:

- If the currently referenced value is a JSON object, the new
  referenced value is the object member with the name identified by
  the reference token. The member name is equal to the token if it
  has the same number of Unicode characters as the token and their
  code points are byte-by-byte equal. No Unicode character
  normalization is performed. If a referenced member name is not
  unique in an object, the member that is referenced is undefined,
  and evaluation fails.
---------------------------------------------------------------------
*/

static int compare(const char *key, const char *path, const char *end)
{
    for (; path < end; key++, path++)
    {
        // '~' in name must match with '~0' in path
        if (*key == '~')
        {
            if ((*path != '~') || (*++path != '0'))
            {
                return 0;
            }
        }
        // '/' in name must match with '~1' in path
        else if (*key == '/')
        {
            if ((*path != '~') || (*++path != '1'))
            {
                return 0;
            }
        }
        // Doesn't match
        else if (*key != *path)
        {
            return 0;
        }
    }
    return *key == '\0';
}

static const json_t *find_key(const json_t *node, const char *path, const char *end)
{
    for (unsigned index = 0; index < node->size; index++)
    {
        if (compare(node->child[index]->key, path, end))
        {
            return node->child[index];
        }
    }
    return NULL;
}

static const json_t *find_index(const json_t *node, const char *path, const char *end)
{
    if (path + strspn(path, "0123456789") != end)
    {
        return NULL;
    }

    unsigned long index = strtoul(path, NULL, 10);

    return index < node->size ? node->child[index] : NULL;
}

static const json_t *pointer(const json_t *node, const char *path)
{
    while (node != NULL)
    {
        const char *end = path + strcspn(path, "/");

        if (node->type == JSON_OBJECT)
        {
            node = find_key(node, path, end);
        }
        else
        {
            node = find_index(node, path, end);
        }
        if (*end != '/')
        {
            return node;
        }
        path = end + 1;
    }
    return NULL;
}

/* Locates a node by path */
json_t *json_pointer(const json_t *node, const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }
    switch (path[0])
    {
        case '/':
            return json_cast(pointer(node, path + 1));
        case '\0':
            return json_cast(node);
        default:
            return NULL;
    }
}

/* Locates a node by path */
json_t *json_extract(const json_pointer_t *pointer)
{
    if ((pointer == NULL) || (pointer->root == NULL))
    {
        return NULL;
    }

    json_t *node = json_cast(pointer->root);

    for (unsigned i = 0; i < pointer->size; i++)
    {
        unsigned index = pointer->path[i];

        if (node->size <= index)
        {
            return NULL;
        }
        node = node->child[index];
    }
    return node;
}

/**
 * Writes an encoded key into the specified buffer in a format compatible
 * with the JSON Pointer specification.
 * The key is prefixed with a forward slash ('/').
 */

static char *write_key(buffer_t *buffer, const char *key)
{
    CHECK(buffer_put(buffer, '/'));

    const char *ptr = key;

    while (*key != '\0')
    {
        switch (*key)
        {
            case '~':
                CHECK(buffer_append(buffer, ptr, (size_t)(key - ptr)));
                CHECK(buffer_append(buffer, "~0", 2));
                ptr = ++key;
                break;
            case '/':
                CHECK(buffer_append(buffer, ptr, (size_t)(key - ptr)));
                CHECK(buffer_append(buffer, "~1", 2));
                ptr = ++key;
                break;
            default:
                key++;
                break;

        }
    }
    return buffer_append(buffer, ptr, (size_t)(key - ptr));
}

/**
 * Writes an index as plain text into the specified buffer.
 * The index is prefixed with a forward slash ('/').
 */
static char *write_index(buffer_t *buffer, unsigned index)
{
    return buffer_format(buffer, "/%u", index);
}

/* Writes a json_pointer into the specified buffer */
char *json_write_pointer(buffer_t *buffer, const json_pointer_t *pointer)
{
    return json_write_pointer_max(buffer, pointer, 0);
}

/* Writes a json_pointer into the specified buffer and limits length */
char *json_write_pointer_max(buffer_t *buffer, const json_pointer_t *pointer,
    size_t max_length)
{
    if ((buffer == NULL) || (pointer == NULL) || (pointer->root == NULL))
    {
        return NULL;
    }
    if (pointer->size == 0)
    {
        return buffer_put(buffer, '/');
    }
    if (max_length == 0)
    {
        max_length = (size_t) -1;
    }

    const json_t *node = pointer->root;
    size_t length = buffer->length;

    for (unsigned i = 0; i < pointer->size; i++)
    {
        unsigned index = pointer->path[i];

        if (node->size <= index)
        {
            return NULL;
        }
        if (node->child[index]->key != NULL)
        {
            CHECK(write_key(buffer, node->child[index]->key));
        }
        else
        {
            CHECK(write_index(buffer, index));
        }
        if (buffer->length - length > max_length)
        {
            buffer_set_length(buffer, length + max_length);
            return buffer_write(buffer, "...");
        }
        node = node->child[index];
    }
    return buffer->text;
}

