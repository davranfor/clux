/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef WORKER_H
#define WORKER_H

#include "buffer.h"

void worker_run(void);
int request_ready(const char *, size_t);
void request_reply(pool_t *, char *, size_t);

#endif

