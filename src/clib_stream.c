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

int file_exists(const char *path)
{
    return access(path, 0) == 0;
}

static char *read_fd(int fd, char *str, size_t length)
{
    if (str == NULL)
    {
        return NULL;
    }

    size_t count = 0;

    while (count < length)
    {
        ssize_t bytes = read(fd, str + count, length - count);

        switch (bytes)
        {
            case -1:
                if (errno == EINTR)
                {
                    continue;
                }
                __attribute__((fallthrough));
            case 0:
                return NULL;
        }
        count += (size_t)bytes;
    }
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
        size_t length = (size_t)st.st_size;
        
        str = malloc(length + 1);
        if (read_fd(fd, str, length))
        {
            str[length] = '\0';
        }
        else
        {
            free(str);
            str = NULL;
        }
    }
    close(fd);
    return str;
}

char *file_read_callback(const char *path, char *(*callback)(void *, size_t),
    void *data)
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
        size_t length = (size_t)st.st_size;

        str = read_fd(fd, callback(data, length), length);
    }
    close(fd);
    return str;
}

int file_write(const char *path, const char *str)
{
    return file_write_bytes(path, str, strlen(str));
}

int file_write_bytes(const char *path, const char *str, size_t length)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd == -1)
    {
        return 0;
    }

    size_t count = 0;

    while (count < length)
    {
        ssize_t bytes = write(fd, str + count, length - count);

        switch (bytes)
        {
            case -1:
                if (errno == EINTR)
                {
                    continue;
                }
                __attribute__((fallthrough));
            case 0:
                close(fd);
                return 0;
        }
        count += (size_t)bytes;
    }
    close(fd);
    return 1;
}

int file_delete(const char *path)
{
    return unlink(path) == 0;
}

