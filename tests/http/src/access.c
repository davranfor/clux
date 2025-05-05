/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include "access.h"

static void generate_key(char *key)
{
    unsigned char bin[AUTH_KEY_SIZE / 2];

    if (RAND_bytes(bin, sizeof bin) != 1)
    {
        fprintf(stderr, "Error generating random bytes\n");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < sizeof bin; i++)
    {
        snprintf(key + (i * 2), 3, "%02x", bin[i]);
    }
    key[AUTH_KEY_SIZE - 1] = '\0';
}

void access_generate_key(auth_t *auth)
{
    generate_key(auth->key);
}

int access_handle(auth_t *auth)
{
    (void)auth;
    return 1;
}

