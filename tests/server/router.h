/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <sys/types.h>
#include "pool.h"

ssize_t router_parse(char *, size_t);
void router_reply(pool_t *, char *, size_t);

#endif

