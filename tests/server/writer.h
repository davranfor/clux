/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef WRITER_H
#define WRITER_H

#include <clux/clib_buffer.h>

void writer_load(void); 
const buffer_t *writer_handle(const char *, const char *, const char *); 

#endif

