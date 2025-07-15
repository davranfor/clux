/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef WRITER_H
#define WRITER_H

#include <clux/clib_buffer.h>
#include <clux/json_header.h>

void writer_load(const char *, const char *);
void writer_reload(const char *, const char *);
const buffer_t *writer_handle(json_t *);

#endif

