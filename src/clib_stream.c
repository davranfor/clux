/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clib_stream.h"

static char *read(FILE *file, size_t length)
{
    char *str = malloc(length + 1);

    if (str != NULL)
    {
        if (fread(str, 1, length, file) == length)
        {
            str[length] = '\0';
        }
        else
        {
            free(str);
            return NULL;
        }
    }
    return str;
}

char *file_read(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        return NULL;
    }

    char *str = NULL;

    if (fseek(file, 0L, SEEK_END) == 0)
    {
        long length = ftell(file);

        if ((length != -1L) && (fseek(file, 0L, SEEK_SET) == 0))
        {
            str = read(file, (size_t)length);
        }
    }
    fclose(file);
    return str;
}

static char *quote(FILE *file, size_t length, const char *prefix, const char *suffix)
{
    size_t prefix_length = strlen(prefix);
    size_t suffix_length = strlen(suffix);
    char *str = malloc(prefix_length + length + suffix_length + 1);

    if (str != NULL)
    {
        memcpy(str, prefix, prefix_length);
        if (fread(str + prefix_length, 1, length, file) != length)
        {
            free(str);
            return NULL;
        }
        memcpy(str + prefix_length + length, suffix, suffix_length + 1);
    }
    return str;
}

char *file_quote(const char *path, const char *prefix, const char *suffix)
{
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        return NULL;
    }

    char *str = NULL;

    if (fseek(file, 0L, SEEK_END) == 0)
    {
        long length = ftell(file);

        if ((length != -1L) && (fseek(file, 0L, SEEK_SET) == 0))
        {
            str = quote(file, (size_t)length, prefix, suffix);
        }
    }
    fclose(file);
    return str;
}

int file_write(const char *path, const char *str)
{
    FILE *file = fopen(path, "w");

    if (file == NULL)
    {
        return 0;
    }

    size_t length = strlen(str);
    int rc = fwrite(str, 1, length, file) == length;

    fclose(file);
    return rc;
}

