/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clib_stream.h"

static char *read(FILE *file, size_t size)
{
    char *str = malloc(size + 1);

    if (str != NULL)
    {
        if (fread(str, 1, size, file) == size)
        {
            str[size] = '\0';
        }
        else
        {
            free(str);
            str = NULL;
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
        long size = ftell(file);

        if ((size != -1L) && (fseek(file, 0L, SEEK_SET) == 0))
        {
            str = read(file, (size_t)size);
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

