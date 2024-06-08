/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "json.h"

struct node
{
    struct node *next;
    json *data;
    char key[];
};

struct json_map
{
    struct node **list;
    json_map *next;
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

json_map *json_map_create(size_t size)
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

    json_map *map = calloc(1, sizeof *map);

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

#define hash_str(key) hash_str((const unsigned char *)(key))
static unsigned long (hash_str)(const unsigned char *key)
{
    unsigned long hash = 5381;
    unsigned char chr;

    while ((chr = *key++))
    {
        hash = ((hash << 5) + hash) + chr;
    }
    return hash;
}

static void reset(json_map *map)
{
    json_map *next = map->next;

    free(map->list);
    map->list = next->list;
    map->room = next->room;
    map->size = next->size;
    map->next = next->next;
    free(next);
}

static void move(json_map *map, struct node *node)
{
    struct node **head = map->list + hash_str(node->key) % map->room;

    node->next = *head;
    *head = node;
    map->size++;
}

static json_map *rehash(json_map *map, unsigned long hash)
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

static struct node *insert(struct node *next, const char *key, json *data)
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

static json *upsert(json_map *map, const char *key, json *data, int replace)
{
    if ((map != NULL) && (data != NULL))
    {
        unsigned long hash = hash_str(key);

        map = rehash(map, hash);

        struct node **head = map->list + hash % map->room;
        struct node *node = *head;

        while (node != NULL)
        {
            if (strcmp(node->key, key) == 0)
            {
                if (replace)
                {
                    json *temp = node->data;

                    node->data = data;
                    return temp;
                }
                else
                {
                    return node->data;
                }
            }
            node = node->next;
        }
        *head = insert(*head, key, data);
        if (*head == NULL)
        {
            return NULL;
        }
        // If more than 75% occupied then create a new table
        if (++map->size > map->room - map->room / 4)
        {
            map->next = json_map_create(map->room);
            if (map->next == NULL)
            {
                return NULL;
            }
        }
        return data;
    }
    return NULL;
}

json *json_map_insert(json_map *map, const char *key, json *data)
{
    return upsert(map, key, data, 0);
}

json *json_map_upsert(json_map *map, const char *key, json *data)
{
    return upsert(map, key, data, 1);
}

json *json_map_delete(json_map *map, const char *key)
{
    if (map != NULL)
    {
        unsigned long hash = hash_str(key);

        map = rehash(map, hash);

        struct node **head = map->list + hash % map->room;
        struct node *node = *head, *prev = NULL;

        while (node != NULL)
        {
            if (strcmp(node->key, key) == 0)
            {
                json *data = node->data;

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

json *json_map_search(const json_map *map, const char *key)
{
    unsigned long hash = hash_str(key);

    while (map != NULL)
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
        // Not found in this table, try in the next one
        map = map->next;
    }
    return NULL;
}

json *json_map_walk(const json_map *map, json_map_walk_callback callback,
    void *cookie)
{
    size_t iter = 0;

    while (map != NULL)
    {
        for (size_t index = 0, size = map->size; size > 0; index++)
        {
            struct node *node = map->list[index];

            while (node != NULL)
            {
                if (!callback(node->data, iter++, cookie))
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

size_t json_map_size(const json_map *map)
{
    size_t size = 0;

    while (map != NULL)
    {
        size += map->size;
        map = map->next;
    }
    return size;
}

void json_map_destroy(json_map *map, void (*callback)(json *))
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

        json_map *next = map->next;

        free(map->list);
        free(map);
        map = next;
    }
}

