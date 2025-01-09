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
#include "clib_unicode.h"
#include "clib_buffer.h"

static char *resize(buffer_t *buffer, size_t size)
{
    if (buffer->fail)
    {
        return NULL;
    }

    char *text = realloc(buffer->text, size);

    if (text == NULL)
    {
        buffer->size = 0;
        buffer->fail = 1;
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

char *buffer_putchr(buffer_t *buffer, char chr)
{
    if (buffer_resize(buffer, 1) == NULL)
    {
        return NULL;
    }
    buffer->text[buffer->length++] = chr;
    buffer->text[buffer->length] = '\0';
    return buffer->text;
}

char *buffer_attach(buffer_t *buffer, const char *text, size_t length)
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

char *buffer_insert(buffer_t *buffer, size_t index,
    const char *text, size_t length)
{
    if (index >= buffer->length)
    {
        return buffer_attach(buffer, text, length);
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
    va_end(args);
    return text;
}

void buffer_adjust(buffer_t *buffer, size_t index)
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
}

