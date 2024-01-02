/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "json_private.h"
#include "json_unicode.h"

/* return 0 if buffer_resize() fails */
#define CHECK(expr) do { if (!(expr)) return 0; } while(0)

static enum json_encode encode = JSON_UTF8;

enum json_encode json_get_encode(void)
{
    return encode;
}

void json_set_encode(enum json_encode value)
{
    encode = value;
}

typedef struct { char *text; size_t length, size; } json_buffer;

static char *buffer_resize(json_buffer *buffer, size_t size)
{
    char *text = realloc(buffer->text, size);

    if (text != NULL)
    {
        buffer->text = text;
        buffer->size = size;
    }
    return text;
}

static size_t buffer_next_size(size_t size)
{
    size--;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    size++;
    return size;
}

static json_buffer *buffer_write_sized(json_buffer *buffer, const char *text,
    size_t length)
{
    size_t size = buffer->length + length + 1;

    if ((size > buffer->size) && !buffer_resize(buffer, buffer_next_size(size)))
    {
        return NULL;
    }
    memcpy(buffer->text + buffer->length, text, length + 1);
    buffer->length += length;
    return buffer;
}

static json_buffer *buffer_write(json_buffer *buffer, const char *text)
{
    return buffer_write_sized(buffer, text, strlen(text));
}

static json_buffer *buffer_write_integer(json_buffer *buffer, double value)
{
    size_t length = (size_t)snprintf(NULL, 0, "%.0f", value);
    size_t size = buffer->length + length + 1;

    if ((size > buffer->size) && !buffer_resize(buffer, buffer_next_size(size)))
    {
        return NULL;
    }
    snprintf(buffer->text + buffer->length, length + 1, "%.0f", value);
    buffer->length += length;
    return buffer;
}

static json_buffer *buffer_write_number(json_buffer *buffer, double value)
{
    size_t length = (size_t)snprintf(NULL, 0, "%.*g", DBL_DECIMAL_DIG, value);
    size_t size = buffer->length + length + 1;

    if ((size > buffer->size) && !buffer_resize(buffer, buffer_next_size(size)))
    {
        return NULL;
    }
    snprintf(buffer->text + buffer->length, length + 1,
             "%.*g", DBL_DECIMAL_DIG, value);

    /* Dot followed by trailing zeros are removed when %g is used */
    size_t end = strspn(buffer->text + buffer->length, "-0123456789");

    buffer->length += length;
    if (end == length)
    {
        return buffer_write(buffer, ".0");
    }
    return buffer;
}

static int buffer_parse(json_buffer *buffer, const char *str)
{
    const char *ptr = str;

    while (*str != '\0')
    {
        char chr = '\0';

        switch (*str)
        {
            case '\b': chr = 'b' ; break;
            case '\f': chr = 'f' ; break;
            case '\n': chr = 'n' ; break;
            case '\r': chr = 'r' ; break;
            case '\t': chr = 't' ; break;
            case '"' : chr = '"' ; break;
            case '\\': chr = '\\'; break;
            default: break;
        }
        if (chr != '\0')
        {
            const char esc[] = {'\\', chr, '\0'};

            CHECK(buffer_write_sized(buffer, ptr, (size_t)(str - ptr)));
            CHECK(buffer_write_sized(buffer, esc, 2));
            ptr = ++str;
        }
        else if (is_cntrl(*str) || ((encode == JSON_ASCII) && !is_ascii(*str)))
        {
            char ucn[sizeof("\\u0123")] = "";
            int length = to_ucn(str, ucn);

            CHECK(buffer_write_sized(buffer, ptr, (size_t)(str - ptr)));
            CHECK(buffer_write_sized(buffer, ucn, 6));
            str += length;
            ptr = str;
        }
        else
        {
            str++;
        }
    }
    CHECK(buffer_write_sized(buffer, ptr, (size_t)(str - ptr)));
    return 1;
}

static int buffer_quote(json_buffer *buffer, const char *text)
{
    CHECK(buffer_write(buffer, "\""));
    CHECK(buffer_parse(buffer, text));
    CHECK(buffer_write(buffer, "\""));
    return 1;
}

static int buffer_print(json_buffer *buffer, const json *node)
{
    switch (node->type)
    {
        case JSON_STRING:
            CHECK(buffer_quote(buffer, node->value.string));
            return 1;
        case JSON_INTEGER:
            CHECK(buffer_write_integer(buffer, node->value.number));
            return 1;
        case JSON_NUMBER:
            CHECK(buffer_write_number(buffer, node->value.number));
            return 1;
        case JSON_BOOLEAN:
            CHECK(buffer_write(buffer, node->value.number != 0 ? "true" : "false"));
            return 1;
        case JSON_NULL:
            CHECK(buffer_write(buffer, "null"));
            return 1;
        default:
            return 0;
    }
}

static int buffer_loop_start(json_buffer *buffer, const json *node,
    int depth, int nested, int indent)
{
    for (int i = 0; i < (depth + nested) * indent; i++)
    {
        CHECK(buffer_write(buffer, " "));
    }
    if (node->name != NULL)
    {
        CHECK(buffer_quote(buffer, node->name));
        CHECK(buffer_write(buffer, indent == 0 ? ":" : ": "));
    }
    switch (node->type)
    {
        case JSON_OBJECT:
            CHECK(buffer_write(buffer, "{"));
            break;
        case JSON_ARRAY:
            CHECK(buffer_write(buffer, "["));
            break;
        default:
            CHECK(buffer_print(buffer, node));
            break;
    }
    if (node->head == NULL)
    {
        switch (node->type)
        {
            case JSON_OBJECT:
                CHECK(buffer_write(buffer, "}"));
                break;
            case JSON_ARRAY:
                CHECK(buffer_write(buffer, "]"));
                break;
            default:
                break;
        }
        if ((depth > 0) && (node->next != NULL))
        {
            CHECK(buffer_write(buffer, ","));
        }
    }
    if (indent > 0)
    {
        CHECK(buffer_write(buffer, "\n"));
    }
    return 1;
}

static int buffer_loop_end(json_buffer *buffer, const json *node,
    int depth, int nested, int indent)
{
    if (node->head != NULL)
    {
        for (int i = 0; i < (depth + nested) * indent; i++)
        {
            CHECK(buffer_write(buffer, " "));
        }
        switch (node->type)
        {
            case JSON_OBJECT:
                CHECK(buffer_write(buffer, "}"));
                break;
            case JSON_ARRAY:
                CHECK(buffer_write(buffer, "]"));
                break;
            default:
                break;
        }
        if ((depth > 0) && (node->next != NULL))
        {
            CHECK(buffer_write(buffer, ","));
        }
        if (indent > 0)
        {
            CHECK(buffer_write(buffer, "\n"));
        }
    }
    return 1;
}

static int buffer_loop(json_buffer *buffer, const json *node, int indent)
{
    int nested = json_key(node) != NULL;
    int depth = 0;

    if (nested)
    {
        CHECK(buffer_write(buffer, indent ? "{\n" : "{"));
    }
    while (node != NULL)
    {
        CHECK(buffer_loop_start(buffer, node, depth, nested, indent));
        if (node->head != NULL)
        {
            node = node->head;
            depth++;
        }
        else if ((depth > 0) && (node->next != NULL))
        {
            node = node->next;
        }
        else
        {
            while (depth-- > 0)
            {
                node = node->parent;
                CHECK(buffer_loop_end(buffer, node, depth, nested, indent));
                if (node->next != NULL)
                {
                    node = node->next;
                    break;
                }
            }
            if (depth <= 0)
            {
                break;
            }
        }
    }
    if (nested)
    {
        CHECK(buffer_write(buffer, indent ? "}\n" : "}"));
    }
    return 1;
}

char *json_encode(const json *node)
{
    json_buffer buffer = {NULL, 0, 0};
    char *text = NULL;

    if (buffer_loop(&buffer, node, 0))
    {
        text = buffer.text;
    }
    else
    {
        free(buffer.text);
    }
    return text;
}

char *json_indent(const json *node, int indent)
{
    json_buffer buffer = {NULL, 0, 0};
    char *text = NULL;

    if (buffer_loop(&buffer, node,
                    indent < 0 ? 0 :
                    indent > 8 ? 8 :
                    indent))
    {
        text = buffer.text;
    }
    else
    {
        free(buffer.text);
    }
    return text;
}

int json_write(const json *node, FILE *file)
{
    if (file == NULL)
    {
        return 0;
    }

    char *str = json_indent(node, 2);

    if (str == NULL)
    {
        return 0;
    }

    int rc = fputs(str, file) != EOF;

    free(str);
    return rc;
}

int json_write_file(const json *node, const char *path)
{
    FILE *file = fopen(path, "w");

    if (file == NULL)
    {
        return 0;
    }

    int rc = 0;
    char *str = json_indent(node, 2);

    if (str != NULL)
    {
        rc = fputs(str, file) != EOF;
        free(str);
    }
    fclose(file);
    return rc;
}

int json_print(const json *node)
{
    return json_write(node, stdout);
}

static int buffer_write_path(json_buffer *buffer, const json *node)
{
    if (node->parent == NULL)
    {
        CHECK(buffer_write(buffer, "$"));
    }
    else
    {
        if (node->name != NULL)
        {
            CHECK(buffer_write(buffer, "[\""));
            CHECK(buffer_parse(buffer, node->name));
            CHECK(buffer_write(buffer, "\"]"));
        }
        if (node->parent->type == JSON_ARRAY)
        {
            char str[16];

            snprintf(str, sizeof str, "[%zu]", json_offset(node));
            CHECK(buffer_write(buffer, str));
        }
    }
    return 1;
}

static int buffer_path(json_buffer *buffer, const json *node)
{
    if (node == NULL)
    {
        return 1;
    }
    if (!buffer_path(buffer, node->parent))
    {
        return 0;
    }
    return buffer_write_path(buffer, node);
}

char *json_path(const json *node)
{
    json_buffer buffer = {NULL, 0, 0};
    char *text = NULL;

    if (buffer_path(&buffer, node))
    {
        text = buffer.text;
    }
    else
    {
        free(buffer.text);
    }
    return text;
}

char *json_quote(const char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    json_buffer buffer = {NULL, 0, 0};
    char *text = NULL;

    if (buffer_quote(&buffer, str))
    {
        text = buffer.text;
    }
    else
    {
        free(buffer.text);
    }
    return text;
}

