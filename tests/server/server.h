/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include "pool.h" 

void server_run(
    uint16_t,
    int (*)(const char *, size_t),
    void (*)(pool_t *, char *, size_t)
);

#endif

