/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/rand.h>
#include "cookie.h"

#define TOKEN_SIZE 65

int cookie_parse(cookie_t *cookie, const char *path, char *str)
{
    if ((str = strstr(str, "Cookie: auth=")))
    {
        str += 13;

        char *end = strchr(str, '\r');

        if (end == NULL)
        {
            return -1;
        }
        *end = '\0';

        int data[2] = {0};

        for (int i = 0; i < 2; i++)
        {
            char *ptr;

            data[i] = (int)strtol(str, &ptr, 10);
            if ((*ptr != ':') || (data[i] <= 0))
            {
                return -1;
            }
            str = ptr + 1;
        }
        if (end - str != TOKEN_SIZE - 1)
        {
            return -1;
        }
        cookie->user = data[0];
        cookie->role = data[1];
        cookie->token = str;
        return 1;
    }
    else if (!strcmp(path, "/login"))
    {
        cookie->token = "";
        return 1;
    }
    else
    {
        return 0;
    }
}

int cookie_create(int user, int role, const char *token, char *cookie)
{
    if (token[0] != '\0')
    {
        snprintf(cookie, COOKIE_SIZE, "%d:%d:%s", user, role, token);
    }
    else
    {
        unsigned char bytes[TOKEN_SIZE / 2];

        if (RAND_bytes(bytes, sizeof(bytes)) != 1)
        {
            return 0;
        }

        char *output = cookie + snprintf(cookie, COOKIE_SIZE, "%d:%d:", user, role);

        for (size_t i = 0; i < sizeof bytes; i++)
        {
            snprintf(output + (i * 2), 3, "%02x", bytes[i]);
        }
        output[TOKEN_SIZE - 1] = '\0';
    }
    return 1;
}

