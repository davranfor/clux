/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

static json_t *find_key(const json_t *node, const char *path, const char *end)
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

static json_t *find_index(const json_t *node, const char *path, const char *end)
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    switch (path[0])
    {
        case '/':
            return (json_t *)pointer(node, path + 1);
        case '\0':
            return (json_t *)node;
        default:
            return NULL;
    }
#pragma GCC diagnostic pop
}

/**
 * Writes an encoded key into the specified buffer in a format compatible
 * with the JSON Pointer specification.
 * The key is prefixed with a forward slash ('/').
 * The buffer must have enough space to hold the resulting key,
 * including the null terminator.
 */
size_t json_pointer_add_key(const char *key, char *buffer, size_t size)
{
    if ((key == NULL) || (size == 0))
    {
        return 0;
    }

    size_t length = 0;

    buffer[length++] = '/';
    while ((*key != '\0') && (length < size - 1))
    {
        switch (*key)
        {
            case '~':
                buffer[length++] = '~';
                buffer[length++] = '0';
                break;
            case '/':
                buffer[length++] = '~';
                buffer[length++] = '1';
                break;
            default:
                buffer[length++] = *key;
                break;
        }
        key++;
    }
    if ((*key != '\0') || (length >= size))
    {
        *buffer = '\0';
        return 0;
    }
    buffer[length] = '\0';
    return length;
}

/**
 * Writes an index as plain text into the specified buffer.
 * The index is prefixed with a forward slash ('/').
 * The buffer must have enough space to hold the resulting index,
 * including the null terminator.
 */
size_t json_pointer_add_index(size_t index, char *buffer, size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    size_t length = 0;

    buffer[length++] = '/';
    size--;
    do
    {
        if (length >= size)
        {
            *buffer = '\0';
            return 0;
        }
        buffer[length++] = '0' + (index % 10);
        index /= 10;
    } while (index > 0);
    buffer[length] = '\0';
    return length;
}

