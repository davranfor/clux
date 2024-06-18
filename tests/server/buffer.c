#include <stdlib.h>
#include <string.h>
#include "buffer.h"

char *pool_set(struct poolfd *pool, char *data, size_t size)
{
    if (pool->type == POOL_ALLOCATED)
    {
        free(pool->data);
    }
    pool->type = POOL_BUFFERED;
    pool->data = data;
    pool->size = size;
    return pool->data;
}

char *pool_add(struct poolfd *pool, const char *data, size_t size)
{
    if (pool->type == POOL_BUFFERED)
    {
        pool->data = NULL;
        pool->size = 0;
    }
    pool->type = POOL_ALLOCATED;

    char *temp = realloc(pool->data, pool->size + size);

    if (temp != NULL)
    {
        pool->data = temp;
        memcpy(pool->data + pool->size, data, size);
        pool->size += size;
    }
    return temp;
}

void pool_sync(struct poolfd *pool, size_t sent)
{
    pool->sent += sent;
}

void pool_reset(struct poolfd *pool)
{
    if (pool->type == POOL_ALLOCATED)
    {
        free(pool->data);
    }
    pool->data = NULL;
    pool->size = 0;
    pool->sent = 0;
    pool->type = 0;
}

