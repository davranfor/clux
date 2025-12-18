/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "clib_stream.h"

static char *read_fd(int fd, size_t length)
{
    char *str = malloc(length + 1);

    if (str == NULL)
    {
        return NULL;
    }

    size_t count = 0;

    while (count < length)
    {
        ssize_t bytes = read(fd, str + count, length - count);

        if (bytes == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            free(str);
            return NULL;
        }
        count += (size_t)bytes;
    }
    str[length] = '\0';
    return str;
}

char *file_read(const char *path)
{
    int fd = open(path, O_RDONLY);

    if (fd < 0)
    {
        return NULL;
    }

    char *str = NULL;
    struct stat st;

    if (fstat(fd, &st) != -1)
    {
        str = read_fd(fd, (size_t)st.st_size);
    }
    close(fd);
    return str;
}

int file_write(const char *path, const char *str)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd == -1)
    {
        return 0;
    }

    size_t length = strlen(str), count = 0;

    while (count < length)
    {
        ssize_t bytes = write(fd, str + count, length - count);

        if (bytes == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            close(fd);
            return 0;
        }
        count += (size_t)bytes;
    }
    close(fd);
    return 1;
}

