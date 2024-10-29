/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef REQUEST_H
#define REQUEST_H

#include "buffer.h"

int request_create_map(void);
int request_ready(const char *, size_t);
void request_reply(struct poolfd *, char *, size_t);

#endif

