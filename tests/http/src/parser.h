/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef PARSER_H
#define PARSER_H

#include <clux/clib_buffer.h>
#include "access.h"

const buffer_t *parser_handle(auth_t *, char *);

#endif

