/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef COOKIE_H
#define COOKIE_H

#include <clux/clib_buffer.h>

#define TOKEN_SIZE 256

typedef struct
{
    double user, role, time;
    char *hmac;
} token_t;

int cookie_parse(const char *, token_t *, char *);
int cookie_handle(token_t *, char *);

#endif

