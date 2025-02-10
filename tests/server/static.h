/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef STATIC_H
#define STATIC_H

#include <clux/clib_buffer.h>

void static_load(char *); 
buffer_t *static_handle(const char *); 
buffer_t *static_error(void); 

#endif

