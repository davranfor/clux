/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef COOKIE_H
#define COOKIE_H

#include <clux/clib_buffer.h>

const buffer_t *cookie_create(int);
char *cookie_parse(char *, int *);

#endif

