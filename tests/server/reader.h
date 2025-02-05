/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef READER_H
#define READER_H

#include "pool.h"

int reader_status(char *, size_t);
void reader_handle(pool_t *, char *, size_t);

#endif

