/*!
 *  \brief     clux - json and json-schema library for C
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
const json_t *schema_get(const char *);
int schema_validate(const json_t *, const json_t *, buffer_t *);

#endif

