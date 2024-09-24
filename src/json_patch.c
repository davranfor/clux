/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "json_private.h"
#include "json_reader.h"
#include "json_writer.h"
#include "json_patch.h"

int json_patch(json_t *target, json_t *source)
{
    if (json_is_object(target) && json_is_object(source))
    {
        size_t size = json_size(source);
        int inserts = 0;

        for (size_t count = 0; count < size; count++)
        {
            json_t *node = json_pop_front(source);

            if (node == NULL)
            {
                return -1;
            }
            if (json_find(source, node->key))
            {
                json_delete(node);
                continue;
            }

            unsigned index = json_index(target, node->key);

            if (index != JSON_NOT_FOUND)
            {
                json_push(target, index, node);
                json_push_back(source, json_pop(target, index + 1));
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

int json_unpatch(json_t *target, json_t *source, int inserts)
{
    if (json_is_object(target) && json_is_object(source))
    {
        json_t *node;

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
            unsigned index = json_index(target, node->key);

            if (index != -1u)
            {
                json_push(target, index, node);
                json_delete(target, index + 1);
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

