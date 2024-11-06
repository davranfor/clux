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

    size_t index = strtoul(path, NULL, 10);

    if (errno == ERANGE)
    {
        errno = 0;
    }
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

