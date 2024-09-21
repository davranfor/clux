/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "clib_math.h"
#include "clib_string.h"
#include "clib_unicode.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_buffer.h"

/**
 * According to IEEE 754-1985, double can represent numbers with maximum
 * accuracy of 17 digits after point. Add to it both minuses for mantissa
 * and period, point, e-char and 3 digits of period (8 bit), and you will
 * get exact 24 chars.
 */
#define MAX_DECIMALS 17
#define NUMBER_CHARS 24

/* return 0 if buffer_realloc() fails */
#define CHECK(expr) do { if (!(expr)) return 0; } while (0)

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

static char *buffer_realloc(json_buffer *buffer, size_t size)
{
    char *text = realloc(buffer->text, size);

    if (text != NULL)
    {
        buffer->text = text;
        buffer->size = size;
    }
    return text;
}

static char *buffer_resize(json_buffer *buffer, size_t length)
{
    size_t size = buffer->length + length + 1;

    if (size > buffer->size)
    {
        return buffer_realloc(buffer, next_pow2(size));
    }
    return buffer->text;
}

static json_buffer *buffer_append(json_buffer *buffer, const char *text,
    size_t length)
{
    CHECK(buffer_resize(buffer, length));
    memcpy(buffer->text + buffer->length, text, length + 1);
    buffer->length += length;
    return buffer;
}

static json_buffer *buffer_write(json_buffer *buffer, const char *text)
{
    return buffer_append(buffer, text, strlen(text));
}

#define buffer_format(buffer, ...) (size_t) \
    snprintf(buffer->text + buffer->length, NUMBER_CHARS + 1, __VA_ARGS__)

static json_buffer *buffer_write_integer(json_buffer *buffer, double value)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = buffer_format(buffer, "%.0f", value);

    buffer->length += length;
    return buffer;
}

static json_buffer *buffer_write_real(json_buffer *buffer, double value)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = buffer_format(buffer, "%.*g", MAX_DECIMALS, value);
    /* Dot followed by trailing zeros are removed when %g is used */
    int done = strspn(buffer->text + buffer->length, "-0123456789") != length;

    buffer->length += length;
    /* Write the fractional part if applicable */
    return done ? buffer : buffer_write(buffer, ".0");
}

static int buffer_parse(json_buffer *buffer, const char *str)
{
    const char *ptr = str;

    while (*str != '\0')
    {
        char esc = encode_esc(str);

        if (esc != '\0')
        {
            const char seq[] = {'\\', esc, '\0'};

            CHECK(buffer_append(buffer, ptr, (size_t)(str - ptr)));
            CHECK(buffer_append(buffer, seq, 2));
            ptr = ++str;
        }
        else if (is_cntrl(*str) || ((encode == JSON_ASCII) && !is_ascii(*str)))
        {
            char seq[sizeof("\\u0123")] = {'\0'};
            size_t length = encode_hex(str, seq);

            CHECK(buffer_append(buffer, ptr, (size_t)(str - ptr)));
            CHECK(buffer_append(buffer, seq, 6));
            str += length;
            ptr = str;
        }
        else
        {
            str++;
        }
    }
    CHECK(buffer_append(buffer, ptr, (size_t)(str - ptr)));
    return 1;
}

static int buffer_quote(json_buffer *buffer, const char *text)
{
    CHECK(buffer_write(buffer, "\""));
    CHECK(buffer_parse(buffer, text));
    CHECK(buffer_write(buffer, "\""));
    return 1;
}

static int buffer_print(json_buffer *buffer, const json_t *node)
{
    switch (node->type)
    {
        case JSON_STRING:
            CHECK(buffer_quote(buffer, node->string));
            return 1;
        case JSON_INTEGER:
            CHECK(buffer_write_integer(buffer, node->number));
            return 1;
        case JSON_REAL:
            CHECK(buffer_write_real(buffer, node->number));
            return 1;
        case JSON_TRUE:
            CHECK(buffer_write(buffer, "true"));
            return 1;
        case JSON_FALSE:
            CHECK(buffer_write(buffer, "false"));
            return 1;
        case JSON_NULL:
            CHECK(buffer_write(buffer, "null"));
            return 1;
        default:
            return 0;
    }
}

static int buffer_print_node(json_buffer *buffer, const json_t *node,
    unsigned short depth, unsigned char indent, unsigned char trailing_comma)
{
    for (unsigned i = 0; i < depth * indent; i++)
    {
        CHECK(buffer_write(buffer, " "));
    }
    if (node->key != NULL)
    {
        CHECK(buffer_quote(buffer, node->key));
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
    if (node->size == 0)
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
        if (trailing_comma != 0)
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

static int buffer_print_pair(json_buffer *buffer, const json_t *node,
    unsigned short depth, unsigned char indent, unsigned char trailing_comma)
{
    if (node->size != 0)
    {
        for (unsigned i = 0; i < depth * indent; i++)
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
        if (trailing_comma != 0)
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

static int buffer_print_tree(json_buffer *buffer,const json_t *node,
    unsigned short depth, unsigned char indent)
{
    for (unsigned index = 0; index < node->size; index++)
    {
        unsigned char trailing_comma = index + 1 < node->size;

        CHECK(buffer_print_node(
            buffer, node->child[index], depth, indent, trailing_comma));
        if (node->child[index]->size > 0)
        {
            CHECK(buffer_print_tree(
                buffer, node->child[index], depth + 1, indent));
            CHECK(buffer_print_pair(
                buffer, node->child[index], depth, indent, trailing_comma));
        }
    }
    return 1;
}

/*
 * The cast from 'const json_t *' to 'json_t *' is needed to set/fake the childs, so
 * we don't need to check if the node has depth 0 on each iteration of print_tree().
 *
 * If the passed node IS a property -----> [{key: value}]
 * If the paseed node IS NOT a property -> [value]
 */ 
static int buffer_loop(json_buffer *buffer, const json_t *node, int indent)
{
    if (node != NULL) 
    {
        int is_property = node->key != NULL;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        const json_t parent =
        {
            .child = (json_t *[]){(json_t *)node},
            .size = 1,
            .type = is_property ? JSON_OBJECT : JSON_ARRAY
        };
        const json_t grandparent =
        {
             .child = (json_t *[]){(json_t *)&parent},
             .size = 1,
             .type = JSON_ARRAY
        };
#pragma GCC diagnostic pop

        return buffer_print_tree(
            buffer, is_property ? &grandparent : &parent, 0, (unsigned char)indent);
    }
    return 0;
}

char *json_encode(const json_t *node)
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

char *json_indent(const json_t *node, int indent)
{
    json_buffer buffer = {NULL, 0, 0};
    char *text = NULL;

    if (buffer_loop(&buffer, node, indent < 0 ? 0 : indent > 8 ? 8 : indent))
    {
        text = buffer.text;
    }
    else
    {
        free(buffer.text);
    }
    return text;
}

int json_write(const json_t *node, FILE *file, int indent)
{
    int rc = 0;

    if (file != NULL)
    {
        char *str = json_indent(node, indent);

        if (str != NULL)
        {
            rc = fputs(str, file) != EOF;
            free(str);
        }
    }
    return rc;
}

int json_write_line(const json_t *node, FILE *file)
{
    int rc = 0;

    if (file != NULL)
    {
        char *str = json_encode(node);

        if (str != NULL)
        {
            rc = fprintf(file, "%s\n", str);
            free(str);
        }
    }
    return rc;
}

int json_write_file(const json_t *node, const char *path, int indent)
{
    FILE *file;
    int rc = 0;

    if ((path != NULL) && (file = fopen(path, "w")))
    {
        char *str = json_indent(node, indent);

        if (str != NULL)
        {
            rc = fputs(str, file) != EOF;
            free(str);
        }
        fclose(file);
    }
    return rc;
}

int json_print(const json_t *node)
{
    return json_write(node, stdout, 2);
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

