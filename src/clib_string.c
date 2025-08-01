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

/* Returns an allocated string using vprintf style */
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

/**
 * memmem implementation
 * Search for a substring (substr) within a larger string (str) given the lengths
 */
char *string_search(const char *str, size_t max, const char *substr, size_t length)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    if (length == 0)
    {
        return (char *)str;
    }
    if (max < length)
    {
        return NULL;
    }
    for (size_t i = 0; i <= max - length; i++)
    {
        if ((str[i] == substr[0]) && !memcmp(str + i, substr, length))
        {
            return (char *)(str + i);
        }
    }
    return NULL;
#pragma GCC diagnostic pop
}

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

/* Returns the number of characters matching 'chr' in a string */
size_t string_count(const char *str, char chr)
{
    size_t count = 0;

    for (; *str != '\0'; str++)
    {
        if (*str == chr)
        {
            count++;
        }
    }
    return count;
}

