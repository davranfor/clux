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
        unsigned count = 0;
        int inserts = 0;

        while (count < source->size)
        {
            const char *key = source->child[count]->key;
            unsigned index = json_index(target, key);

            if (index == JSON_NOT_FOUND)
            {
                // If realloc fails, undo changes
                if (!json_move(target, JSON_TAIL, source, count))
                {
                    while (json_delete(source, count));
                    json_unpatch(target, source, inserts);
                    return -1;
                }
                inserts++;
            }
            else
            {
                json_swap(target, index, source, count);
                // Delete repeated keys in the list
                index = json_index(source, key);
                if (index != count)
                {
                    json_swap(source, index, count);
                    json_delete(source, index);
                }
                else
                {
                    count++;
                }
            }
        }
        return inserts;
    }
    return -1;
}

void json_unpatch(json_t *target, json_t *source, int inserts)
{
    if (json_is_object(target) && json_is_object(source))
    {
        while (source->size > 0)
        {
            json_t *node = source->child[source->size - 1];
            unsigned index = json_index(target, node->key);

            if (index != JSON_NOT_FOUND)
            {
                json_swap(target, index, source, JSON_TAIL);
            }
            json_delete_back(source);
        }
        while ((inserts-- > 0) && json_delete_back(target));
    }
}

