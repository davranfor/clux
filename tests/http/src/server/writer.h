/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef WRITER_H
#define WRITER_H

#include <clux/clib_buffer.h>
#include <clux/json_header.h>

void writer_load(void); 
void writer_reload(void); 
const buffer_t *writer_handle(json_t *); 

#endif

