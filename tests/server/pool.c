/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "pool.h"

void pool_bind(pool_t *pool, char *text, size_t length)
{
    if (pool->type == POOL_ALLOCATED)
    {
        free(pool->text);
    }
    pool->text = text;
    pool->length = length;
    pool->type = POOL_BUFFERED;
}

char *pool_put(pool_t *pool, const char *text, size_t length)
{
    if (pool->type == POOL_BUFFERED)
    {
        pool->text = NULL;
        pool->length = 0;
    }
    pool->type = POOL_ALLOCATED;

    char *temp = realloc(pool->text, pool->length + length + 1);

    if (temp != NULL)
    {
        pool->text = temp;
        memcpy(pool->text + pool->length, text, length);
        pool->text[pool->length + length] = '\0';
        pool->length += length;
    }
    return temp;
}

void pool_sync(pool_t *pool, size_t sent)
{
    pool->sent += sent;
}

void pool_reset(pool_t *pool)
{
    if (pool->type == POOL_ALLOCATED)
    {
        free(pool->text);
    }
    pool->text = NULL;
    pool->length = 0;
    pool->sent = 0;
    pool->type = 0;
}

