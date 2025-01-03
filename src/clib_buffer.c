/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "clib_math.h"
#include "clib_buffer.h"

static char *resize(buffer_t *buffer, size_t size)
{
    char *text = realloc(buffer->text, size);

    if (text != NULL)
    {
        buffer->text = text;
        buffer->size = size;
    }
    return text;
}

char *buffer_resize(buffer_t *buffer, size_t length)
{
    size_t size = buffer->length + length + 1;

    if (size > buffer->size)
    {
        return resize(buffer, next_pow2(size));
    }
    return buffer->text;
}

char *buffer_putchr(buffer_t *buffer, char chr)
{
    if (buffer_resize(buffer, 1) == NULL)
    {
        return NULL;
    }
    buffer->text[buffer->length] = chr;
    buffer->length += 1;
    return buffer->text;
}

char *buffer_attach(buffer_t *buffer, const char *text, size_t length)
{
    if (buffer_resize(buffer, length) == NULL)
    {
        return NULL;
    }
    memcpy(buffer->text + buffer->length, text, length + 1);
    buffer->length += length;
    return buffer->text;
}

char *buffer_insert(buffer_t *buffer, size_t offset, char *text)
{
    if (offset >= buffer->length)
    {
        return buffer_append(buffer, text);
    }

    size_t length = strlen(text);
    
    if (buffer_resize(buffer, length) == NULL)
    {
        return NULL;
    }
    memmove(buffer->text + offset + length,
            buffer->text + offset,
            buffer->length - offset);
    memcpy(buffer->text + offset, text, length);
    buffer->length += length;
    return buffer->text;
}

char *buffer_append(buffer_t *buffer, const char *text)
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

char *buffer_format(buffer_t *buffer, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    size_t length = (size_t)vsnprintf(NULL, 0, fmt, args);

    va_end(args);
    if (buffer_resize(buffer, length) == NULL)
    {
        return NULL;
    }
    va_start(args, fmt);
    vsnprintf(buffer->text + buffer->length, length + 1, fmt, args);
    va_end(args);
    buffer->length += length;
    return buffer->text;
}

