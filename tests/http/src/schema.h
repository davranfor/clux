/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include <clux/clib_buffer.h>
#include <clux/json_header.h>

void schema_load(void);
void schema_reload(void);
int schema_add(const char *);
int schema_validate(json_t *, buffer_t *);

#endif

