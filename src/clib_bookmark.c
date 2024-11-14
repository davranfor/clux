/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <stdint.h>
#include "clib_bookmark.h"

struct node
{
    struct node *next;
    const void *data;
};

struct bookmark
{
    struct node **list;
    bookmark_t *next;
    size_t room;
    size_t size;
};

static const size_t primes[] =
{
    53,        97,         193,        389,
    769,       1543,       3079,       6151,
    12289,     24593,      49157,      98317,
    196613,    393241,     786433,     1572869,
    3145739,   6291469,    12582917,   25165843,
    50331653,  100663319,  201326611,  402653189,
    805306457, 1610612741, 3221225473, 4294967291
};

bookmark_t *bookmark_create(size_t size)
{
    enum {NPRIMES = sizeof primes / sizeof *primes};

    for (size_t iter = 0; iter < NPRIMES; iter++)
    {
        if (size < primes[iter])
        {
            size = primes[iter];
            break;
        }
    }

    bookmark_t *map = calloc(1, sizeof *map);

    if (map != NULL)
    {
        map->list = calloc(size, sizeof *map->list);
        if (map->list == NULL)
        {
            free(map);
            return NULL;
        }
        map->room = size;
    }
    return map;
}

#define hash(key) hash_address(key)
static unsigned long hash_address(const void *data)
{
    uintptr_t key = (uintptr_t)data;

    if (sizeof(uintptr_t) <= 4)
    {
        key = ((key >> 16) ^ key) * 0x45d9f3b;
        key = ((key >> 16) ^ key) * 0x45d9f3b;
        key = ((key >> 16) ^ key);
    }
    else
    {
        key = (key ^ (key >> 30)) * 0xbf58476d1ce4e5b9;
        key = (key ^ (key >> 27)) * 0x94d049bb133111eb;
        key = (key ^ (key >> 31));
    }
    return key;
}

static void reset(bookmark_t *map)
{
    bookmark_t *next = map->next;

    free(map->list);
    map->list = next->list;
    map->room = next->room;
    map->size = next->size;
    map->next = next->next;
    free(next);
}

static void move(bookmark_t *map, struct node *node)
{
    struct node **head = map->list + hash(node->data) % map->room;

    node->next = *head;
    *head = node;
    map->size++;
}

static bookmark_t *rehash(bookmark_t *map, unsigned long hash)
{
    while (map->next != NULL)
    {
        struct node **head = map->list + hash % map->room;
        struct node *node = *head;

        *head = NULL;
        while (node != NULL)
        {
            struct node *next = node->next;

            move(map->next, node);
            map->size--;
            node = next;
        }
        if (map->size == 0)
        {
            reset(map);
        }
        else
        {
            map = map->next;
        }
    }
    return map;
}

static struct node *push_node(struct node *next, const void *data)
{
    struct node *node = malloc(sizeof *node);

    if (node != NULL)
    {
        node->next = next;
        node->data = data;
    }
    return node;
}

const void *bookmark_insert(bookmark_t *map, const void *data)
{
    if ((map != NULL) && (data != NULL))
    {
        unsigned long hash = hash(data);

        map = rehash(map, hash);

        struct node **head = map->list + hash % map->room;
        struct node *node = *head;

        while (node != NULL)
        {
            if (node->data == data)
            {
                return data;
            }
            node = node->next;
        }
        if ((*head = push_node(*head, data)) != NULL)
        {
            if (++map->size > map->room - map->room / 4)
            {
                map->next = bookmark_create(map->room);
                if (map->next == NULL)
                {
                    return NULL;
                }
            }
            return data;
        }
    }
    return NULL;
}

const void *bookmark_delete(bookmark_t *map, const void *data)
{
    if ((map != NULL) && (data != NULL))
    {
        unsigned long hash = hash(data);

        map = rehash(map, hash);

        struct node **head = map->list + hash % map->room;
        struct node *node = *head, *prev = NULL;

        while (node != NULL)
        {
            if (node->data == data)
            {
                if (prev != NULL)
                {
                    prev->next = node->next;
                }
                else
                {
                    *head = node->next;
                }
                free(node);
                map->size--;
                return data;
            }
            prev = node;
            node = node->next;
        }
    }
    return NULL;
}

const void *bookmark_search(const bookmark_t *map, const void *data)
{
    if ((map != NULL) && (data != NULL))
    {
        unsigned long hash = hash(data);

        do
        {
            const struct node *node = map->list[hash % map->room];

            while (node != NULL)
            {
                if (node->data == data)
                {
                    return data;
                }
                node = node->next;
            }
        } while ((map = map->next));

    }
    return NULL;
}

const void *bookmark_walk(const bookmark_t *map,
    bookmark_callback callback, void *data)
{
    size_t iter = 0;

    while (map != NULL)
    {
        for (size_t index = 0, size = map->size; size > 0; index++)
        {
            struct node *node = map->list[index];

            while (node != NULL)
            {
                if (!callback(node->data, iter++, data))
                {
                    return node->data;
                }
                node = node->next;
                size--;
            }
        }
        map = map->next;
    }
    return NULL;
}

size_t bookmark_size(const bookmark_t *map)
{
    size_t size = 0;

    while (map != NULL)
    {
        size += map->size;
        map = map->next;
    }
    return size;
}

void bookmark_destroy(bookmark_t *map)
{
    while (map != NULL)
    {
        bookmark_t *next = map->next;

        free(map->list);
        free(map);
        map = next;
    }
}

