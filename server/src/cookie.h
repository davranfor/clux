/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef COOKIE_H
#define COOKIE_H

#include <clux/clib_buffer.h>

#define COOKIE_SIZE 128

enum {COOKIE_USER, COOKIE_ROLE, COOKIE_TOKEN};

typedef struct
{
    int user, role;
    char *token;
} cookie_t;

int cookie_parse(cookie_t *, const char *, char *);
int cookie_create(int, int, const char *, char *);

#endif

