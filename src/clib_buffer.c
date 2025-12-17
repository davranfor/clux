/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "clib_math.h"
#include "clib_unicode.h"
#include "clib_buffer.h"

buffer_t *buffer_create(void)
{
    return calloc(1, sizeof(buffer_t));
}

static char *resize(buffer_t *buffer, size_t size)
{
    if (buffer->error)
    {
        return NULL;
    }

    char *text = realloc(buffer->text, size);

    if (text == NULL)
    {
        buffer_set_error(buffer, BUFFER_ERROR_RESIZE);
        return NULL;
    }
    buffer->text = text;
    buffer->size = size;
    return text;
}

char *buffer_resize(buffer_t *buffer, size_t length)
{
    size_t size = buffer->length + length + 1;

    if ((size > buffer->size) || (buffer->size == 0))
    {
        return resize(buffer, next_pow2(size));
    }
    return buffer->text;
}

char *buffer_repeat(buffer_t *buffer, char chr, size_t count)
{
    if (buffer_resize(buffer, count) == NULL)
    {
        return NULL;
    }
    memset(buffer->text + buffer->length, chr, count);
    buffer->text[buffer->length + count] = '\0';
    buffer->length += count;
    return buffer->text;
}

char *buffer_insert(buffer_t *buffer, size_t index, const char *text, size_t length)
{
    if (index >= buffer->length)
    {
        return buffer_append(buffer, text, length);
    }
    if (buffer_resize(buffer, length) == NULL)
    {
        return NULL;
    }
    memmove(buffer->text + index + length,
            buffer->text + index,
            buffer->length - index + 1);
    memcpy(buffer->text + index, text, length);
    buffer->length += length;
    return buffer->text;
}

char *buffer_append(buffer_t *buffer, const char *text, size_t length)
{
    if (buffer_resize(buffer, length) == NULL)
    {
        return NULL;
    }
    memcpy(buffer->text + buffer->length, text, length);
    buffer->text[buffer->length + length] = '\0';
    buffer->length += length;
    return buffer->text;
}

char *buffer_delete(buffer_t *buffer, size_t index, size_t length)
{
    if ((buffer->length == 0) || (index + length > buffer->length))
    {
        return buffer->text;
    }
    memmove(buffer->text + index,
            buffer->text + index + length,
            buffer->length - index - length + 1);
    buffer->length -= length;
    return buffer->text;
}

char *buffer_format(buffer_t *buffer, const char *fmt, ...)
{
    va_list args, copy;

    va_start(args, fmt);
    va_copy(copy, args);

    int bytes = vsnprintf(NULL, 0, fmt, copy);

    va_end(copy);

    size_t length = (size_t)bytes;
    char *text = NULL;

    if ((bytes >= 0) && (text = buffer_resize(buffer, length)))
    {
        vsnprintf(text + buffer->length, length + 1, fmt, args);
        buffer->length += length;
    }
    else
    {
        buffer_set_error(buffer, BUFFER_ERROR_FORMAT);
    }
    va_end(args);
    return text;
}

char *buffer_write(buffer_t *buffer, const char *text)
{
    size_t length = strlen(text);

    if (buffer_resize(buffer, length) == NULL)
    {
        return NULL;
    }
    memcpy(buffer->text + buffer->length, text, length + 1);
    buffer->length += length;
    return buffer->text;
}

char *buffer_put(buffer_t *buffer, char chr)
{
    if (buffer_resize(buffer, 1) == NULL)
    {
        return NULL;
    }
    buffer->text[buffer->length++] = chr;
    buffer->text[buffer->length] = '\0';
    return buffer->text;
}

char *buffer_set_length(buffer_t *buffer, size_t index)
{
    if ((index <= buffer->length) && (buffer->text != NULL))
    {
        while ((index > 0) && !is_utf8(buffer->text[index]))
        {
            index--;
        }
        buffer->text[index] = '\0';
        buffer->length = index;
    }
    return buffer->text;
}

void buffer_set_error(buffer_t *buffer, int error)
{
    if ((buffer->error == 0) || (error == 0))
    {
        free(buffer->text);
        buffer->text = NULL;
        buffer->length = 0;
        buffer->size = 0;
        buffer->error = error;
    }
}

void buffer_reset(buffer_t *buffer)
{
    buffer_set_length(buffer, 0);
    buffer->error = 0;
}

void buffer_clean(buffer_t *buffer)
{
    free(buffer->text);
    buffer->text = NULL;
    buffer->length = 0;
    buffer->size = 0;
    buffer->error = 0;
}

void buffer_destroy(buffer_t *buffer)
{
    if (buffer != NULL)
    {
        free(buffer->text);
        free(buffer);
    }
}

void buffer_free(void *buffer)
{
    buffer_destroy(buffer);
}

