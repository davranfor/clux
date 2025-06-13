/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clib_unicode.h"
#include "clib_string.h"

/* Returns a duplicate of the string */
char *string_clone(const char *str)
{
    size_t size = strlen(str) + 1;
    char *ptr = malloc(size);

    if (ptr != NULL)
    {
        memcpy(ptr, str, size);
    }
    return ptr;
}

/* Returns an allocated string using printf style*/
char *string_format(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    char *str = string_vprint(fmt, args);

    va_end(args);
    return str;
}

/* Returns an allocated strung using vprintf style */
char *string_vprint(const char *fmt, va_list args)
{
    va_list copy;

    va_copy(copy, args);

    int bytes = vsnprintf(NULL, 0, fmt, copy);
 
    va_end(copy);

    if (bytes < 0)
    {
        return NULL;
    }

    size_t size = (size_t)bytes + 1;
    char *str = malloc(size);

    if (str != NULL)
    {
        vsnprintf(str, size, fmt, args);
    }
    return str;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
/* Search for a substring (b) within a larger string (a) given the lengths */
char *string_search(const char *a, size_t na, const char *b, size_t nb)
{
    if (nb == 0)
    {
        return (char *)a;
    }
    if (na < nb)
    {
        return NULL;
    }

    const char c = b[0];

    for (size_t i = 0; i <= na - nb; i++)
    {
        if ((a[i] == c) && !memcmp(a + i, b, nb))
        {
            return (char *)(a + i);
        }
    }
    return NULL;
}
#pragma GCC diagnostic pop

/* Returns the number of multibytes of a string */
size_t string_length(const char *str)
{
    size_t length = 0;

    for (; *str != '\0'; str++)
    {
        if (is_utf8(*str))
        {
            length++;
        }
    }
    return length;
}

