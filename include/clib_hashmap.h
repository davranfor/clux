/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_HASHMAP_H
#define CLIB_HASHMAP_H

typedef struct map map_t;
typedef int (*map_callback)(void *, size_t, void *);

map_t *map_create(size_t);
void *map_update(map_t *, const char *, void *);
void *map_insert(map_t *, const char *, void *);
void *map_upsert(map_t *, const char *, void *);
void *map_delete(map_t *, const char *);
void *map_search(const map_t *, const char *);
void *map_search_max(const map_t *, const char *, size_t);
void *map_walk(const map_t *, map_callback, void *);
size_t map_size(const map_t *);
void map_destroy(map_t *, void (*)(void *));

#endif

