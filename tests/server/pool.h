/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef POOL_H
#define POOL_H

typedef struct
{
    char *text;
    size_t length, sent;
    unsigned type;
} pool_t;

enum {POOL_BUFFERED = 1, POOL_ALLOCATED};

char *pool_set(pool_t *, char *, size_t);
char *pool_put(pool_t *, const char *, size_t);
void pool_sync(pool_t *, size_t);
void pool_reset(pool_t *);

#endif

