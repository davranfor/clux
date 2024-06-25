/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "json_private.h"

static int compare(const char *name, const char *path, const char *end)
{
    for (; path < end; name++, path++)
    {
        // '~' in name must match with '~0' in path
        if (*name == '~')
        {
            if ((*path != '~') || (*++path != '0'))
            {
                return 0;
            }
        }
        // '/' in name must match with '~1' in path
        else if (*name == '/')
        {
            if ((*path != '~') || (*++path != '1'))
            {
                return 0;
            }
        }
        // Doesn't match
        else if (*name != *path)
        {
            return 0;
        }
    }
    return (*name == '\0');
}

static json *get_by_name(const json *root, const char *path, const char *end)
{
    for (json *node = root->head; node != NULL; node = node->next)
    {
        assert(node->name != NULL);
        if (compare(node->name, path, end))
        {
            return node;
        }
    }
    return NULL;
}

static json *get_by_item(const json *root, const char *path, const char *end)
{
    if (path + strspn(path, "0123456789") != end)
    {
        return NULL;
    }
    return json_at(root, strtoul(path, NULL, 10));
}

static const char *next_path(const char *path)
{
    return path + strcspn(path, "/");
}

/* json_pointer helper */
static json *pointer(json *node, const char *path)
{
    while ((node != NULL) && (*path != '\0'))
    {
        const char *end = next_path(path);

        node = (node->type == JSON_OBJECT)
            ? get_by_name(node, path, end)
            : get_by_item(node, path, end);
        path = *end ? end + 1 : end;
    }
    return node;
}

/* Locates a node by path */
json *json_pointer(const json *node, const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }
    return (*path == '/')
        ? pointer(json_root(node), path + 1)
        : pointer(json_self(node), path);
}

