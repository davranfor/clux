/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include "clib_math.h"

/* Random value between 0 and range - 1 */
int rrand(int range)
{
    return (int)((double)range * (rand() / (RAND_MAX + 1.0)));
}

/* Reads 'size' cryptographically secure random bytes from /dev/urandom into 'buffer' */
int rand_bytes(unsigned char *buffer, size_t size)
{
    FILE *file = fopen("/dev/urandom", "rb");

    if (file == NULL)
    {
        return 0;
    }
    
    int rc = fread(buffer, 1, size, file) == size;

    fclose(file);
    return rc;
}

/**
 * Generates a cryptographically secure random password.
 * 'password' is a buffer where the generated password will be stored.
 * 'size' is the total size of the buffer, including space for the null terminator.
 */
int rand_password(char *password, size_t size)
{
    unsigned char *buffer = (unsigned char *)password;

    if ((size-- == 0) || !rand_bytes(buffer, size))
    {
        return 0;
    }

    const char charset[] = "abcdefghijklmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "0123456789"
                           "!@#$%^&*()-_=+[]{}|;:,.<>?";

    for (size_t i = 0; i < size; i++)
    {
        password[i] = charset[buffer[i] % (sizeof(charset) - 1)];
    }
    password[size] = '\0';
    return 1;
}

/* Generates a 64-bit number by computing the FNV-1a hash of 'input' */
uint64_t fnv1a_64(const char *input, size_t length)
{
    uint64_t hash = 1469598103934665603ULL;

    for (size_t i = 0; i < length; i++)
    {
        hash ^= (uint8_t)input[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

/* Returns the smallest power of 2 that is greater than or equal to size */
size_t next_pow2(size_t number)
{
    number -= 1;
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    if (sizeof(number) > 4)
    {
        number |= number >> 32;
    }
    number += 1;
    return number;
}

/* Returns the next power of 2 if 'size' is a power of 2, 'size' otherwise */
unsigned next_uint(unsigned size)
{
    return size == 0 ? 1 : (size & (size - 1)) ? size : size << 1;
}
size_t next_ulong(size_t size)
{
    return size == 0 ? 1 : (size & (size - 1)) ? size : size << 1;
}

