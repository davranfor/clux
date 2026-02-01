/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/*
--------------------------------------------------------
Map of key/value pairs
--------------------------------------------------------
Keys (strings) are copied as a flexible array member
Values are references to data (generic type void *)

- Uses Dan Berstein's djb2 algorithm as hash function
  http://www.cse.yorku.ca/~oz/hash.html
- Nodes that colides are handled in a singly linked list
- The size of the map is duplicated when 75% is occupied
- Nodes in old maps are moved (rehashed) to new maps
  on insertion/deletion (search doesn't alter the map)
--------------------------------------------------------
*/

#include <stdlib.h>
#include <string.h>
#include "clib_hashmap.h"

struct node
{
    struct node *next;
    void *data;
    char key[];
};

struct map
{
    struct node **list;
    map_t *next;
    size_t room;
    size_t size;
};

enum { UPDATE, INSERT, UPSERT };

map_t *map_create(size_t size)
{
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
    enum { NPRIMES = sizeof primes / sizeof *primes };

    for (size_t i = 0; i < NPRIMES; i++)
    {
        if (size < primes[i])
        {
            size = primes[i];
            break;
        }
    }

    map_t *map = calloc(1, sizeof *map);

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

#define hash(key) hash_str((const unsigned char *)(key))
static unsigned long hash_str(const unsigned char *key)
{
    unsigned long hash = 5381;
    unsigned char chr;

    while ((chr = *key++))
    {
        hash = ((hash << 5) + hash) + chr;
    }
    return hash;
}

#define hash_max(key, length) hash_max_str((const unsigned char *)(key), (length))
static unsigned long hash_max_str(const unsigned char *key, size_t length)
{
    unsigned long hash = 5381;
    unsigned char chr;
    size_t count = 0;

    while ((chr = *key++) && (count < length))
    {
        hash = ((hash << 5) + hash) + chr;
        count++;
    }
    return hash;
}

static void reset(map_t *map)
{
    map_t *next = map->next;

    free(map->list);
    map->list = next->list;
    map->room = next->room;
    map->size = next->size;
    map->next = next->next;
    free(next);
}

static void move(map_t *map, struct node *node)
{
    struct node **head = map->list + hash(node->key) % map->room;

    node->next = *head;
    *head = node;
    map->size++;
}

static map_t *rehash(map_t *map, unsigned long hash)
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

static struct node *prepend(struct node *next, const char *key, void *data)
{
    size_t size = strlen(key) + 1;
    struct node *node = malloc(sizeof *node + size);

    if (node != NULL)
    {
        node->next = next;
        node->data = data;
        memcpy(node->key, key, size);
    }
    return node;
}

static void *apply(map_t *map, const char *key, void *data, int request)
{
    if ((map == NULL) || (key == NULL) || (data == NULL))
    {
        return NULL;
    }

    unsigned long hash = hash(key);

    map = rehash(map, hash);

    struct node **head = map->list + hash % map->room;
    struct node *node = *head;

    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            void *result = node->data;

            if (request != INSERT)
            {
                node->data = data;
            }
            return result;
        }
        node = node->next;
    }
    if (request == UPDATE)
    {
        return NULL;
    }
    if ((*head = prepend(*head, key, data)) != NULL)
    {
        if (++map->size > map->room - map->room / 4)
        {
            map->next = map_create(map->room);
            if (map->next == NULL)
            {
                return NULL;
            }
        }
        return data;
    }
    return NULL;
}

void *map_update(map_t *map, const char *key, void *data)
{
    return apply(map, key, data, UPDATE);
}

void *map_insert(map_t *map, const char *key, void *data)
{
    return apply(map, key, data, INSERT);
}

void *map_upsert(map_t *map, const char *key, void *data)
{
    return apply(map, key, data, UPSERT);
}

void *map_delete(map_t *map, const char *key)
{
    if ((map == NULL) || (key == NULL))
    {
        return NULL;
    }

    unsigned long hash = hash(key);

    map = rehash(map, hash);

    struct node **head = map->list + hash % map->room;
    struct node *node = *head, *prev = NULL;

    while (node != NULL)
    {
        if (strcmp(node->key, key) == 0)
        {
            void *data = node->data;

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
    return NULL;
}

void *map_search(const map_t *map, const char *key)
{
    if ((map == NULL) || (key == NULL))
    {
        return NULL;
    }

    unsigned long hash = hash(key);

    do
    {
        const struct node *node = map->list[hash % map->room];

        while (node != NULL)
        {
            if (strcmp(node->key, key) == 0)
            {
                return node->data;
            }
            node = node->next;
        }
    } while ((map = map->next));
    return NULL;
}

void *map_search_max(const map_t *map, const char *key, size_t length)
{
    if ((map == NULL) || (key == NULL))
    {
        return NULL;
    }

    unsigned long hash = hash_max(key, length);

    do
    {
        const struct node *node = map->list[hash % map->room];

        while (node != NULL)
        {
            if ((strncmp(node->key, key, length) == 0) &&
                (node->key[length] == '\0'))
            {
                return node->data;
            }
            node = node->next;
        }
    } while ((map = map->next));
    return NULL;
}

void *map_walk(const map_t *map, map_callback callback, void *data)
{
    if (callback == NULL)
    {
        return NULL;
    }
    while (map != NULL)
    {
        for (size_t index = 0, size = map->size; size > 0; index++)
        {
            struct node *node = map->list[index];

            while (node != NULL)
            {
                if (!callback(node->key, node->data, data))
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

size_t map_size(const map_t *map)
{
    size_t size = 0;

    while (map != NULL)
    {
        size += map->size;
        map = map->next;
    }
    return size;
}

void map_destroy(map_t *map, void (*callback)(void *))
{
    while (map != NULL)
    {
        for (size_t index = 0; map->size > 0; index++)
        {
            struct node *node = map->list[index];

            while (node != NULL)
            {
                struct node *next = node->next;

                if (callback != NULL)
                {
                    callback(node->data);
                }
                free(node);
                node = next;
                map->size--;
            }
        }

        map_t *next = map->next;

        free(map->list);
        free(map);
        map = next;
    }
}

