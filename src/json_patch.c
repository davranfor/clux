/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "json_private.h"
#include "json_reader.h"
#include "json_writer.h"
#include "json_patch.h"

int json_patch(json_t *source, json_t *target)
{
    if (json_is_object(source) && json_is_object(target))
    {
        unsigned count = 0;
        int inserts = 0;

        while (count < source->size)
        {
            const char *key = source->child[count]->key;
            unsigned index = json_index(target, key);

            if (index != JSON_NOT_FOUND)
            {
                json_swap(source, count, target, index);
                index = json_index(source, key);
                if (index != count)
                {
                    // Delete repeated keys in the list
                    json_swap(source, index, count);
                    json_delete(source, index);
                }
                else
                {
                    count++;
                }
            }
            else
            {
                if (!json_move(source, count, target, JSON_TAIL))
                {
                    // Undo changes
                    while (json_delete(source, count));
                    json_unpatch(source, target, inserts);
                    return -1;
                }
                inserts++;
            }
        }
        return inserts;
    }
    return -1;
}

void json_unpatch(json_t *source, json_t *target, int inserts)
{
    if (json_is_object(source) && json_is_object(target))
    {
        while (source->size > 0)
        {
            const char *key = source->child[source->size - 1]->key;
            unsigned index = json_index(target, key);

            if (index != JSON_NOT_FOUND)
            {
                json_swap(source, JSON_TAIL, target, index);
            }
            json_delete_back(source);
        }
        while ((inserts-- > 0) && json_delete_back(target));
    }
}

