/*!
 *  \brief     clux - void and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_BOOKMARK_H
#define CLIB_BOOKMARK_H

typedef struct bookmark bookmark_t;
typedef int (*bookmark_callback)(const void *, size_t, void *);

bookmark_t *bookmark_create(size_t);
const void *bookmark_insert(bookmark_t *, const void *);
const void *bookmark_delete(bookmark_t *, const void *);
const void *bookmark_search(const bookmark_t *, const void *);
const void *bookmark_walk(const bookmark_t *, bookmark_callback, void *);
size_t bookmark_size(const bookmark_t *);
void bookmark_destroy(bookmark_t *);

#endif

