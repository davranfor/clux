/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef COOKIE_H
#define COOKIE_H

#include <clux/clib_buffer.h>

typedef struct
{
    double user, role, timestamp;
    char *token;
} session_t;

const buffer_t *cookie_create(int);
int cookie_parse(char *, int *);

#endif

