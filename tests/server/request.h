/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef REQUEST_H
#define REQUEST_H

#include "pool.h"

ssize_t request_handle(const char *, size_t);
void request_reply(pool_t *, char *, size_t);

#endif

