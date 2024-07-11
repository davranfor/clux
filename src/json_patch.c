/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "json_private.h"
#include "json_reader.h"
#include "json_writer.h"
#include "json_patch.h"

int json_patch(json *target, json *source)
{
    if (json_is_object(target) && json_is_object(source))
    {
        size_t size = json_size(source);
        int inserts = 0;

        for (size_t count = 0; count < size; count++)
        {
            json *node = json_pop_front(source);

            if (json_find(source, node->name))
            {
                json_delete(node);
                continue;
            }

            json *item = json_find(target, node->name);

            if (item != NULL)
            {
                json_push_before(item, node);
                json_pop(item);
                json_push_back(source, item);
            }
            else
            {
                json_push_back(target, node);
                inserts++;
            }
        }
        return inserts;
    }
    return -1;
}

int json_unpatch(json *target, json *source, int inserts)
{
    if (json_is_object(target) && json_is_object(source))
    {
        json *node;

        for (int count = 0; count < inserts; count++)
        {
            if ((node = json_pop_back(target)))
            {
                json_delete(node);
            }
            else
            {
                return -1;
            }
        }
        while ((node = json_pop_front(source)))
        {
            json *item = json_find(target, node->name);

            if (item != NULL)
            {
                json_push_before(item, node);
                json_delete(item);
            }
            else
            {
                json_delete(node);
                return -1;
            }
        }
        return 0;
    }
    return -1;
}

