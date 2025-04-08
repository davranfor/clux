/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef STATIC_H
#define STATIC_H

#include <clux/clib_buffer.h>

void static_load(void);
void static_reload(void);
int static_add(const char *);
const buffer_t *static_get(const char *);
const buffer_t *static_no_content(void);
const buffer_t *static_bad_request(void);
const buffer_t *static_not_found(void);

#endif

