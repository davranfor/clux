/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include "cookie.h"

const buffer_t *cookie_create(int user_id)
{
    (void)user_id;
    return NULL;
}

char *cookie_parse(char *token, int *user_id)
{
    (void)user_id;
    return token;
}

