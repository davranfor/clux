/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef SCHEMA_H
#define SCHEMA_H

#include <clux/clib_hashmap.h>

void schema_load(void);
map_t *schema_map(void);

#endif

