/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "clib_check.h"
#include "clib_unicode.h"
#include "json_private.h"
#include "json_buffer.h"

/**
 * The 'encode' static global variable can take the following values:
 * - JSON_ASCII: Escapes control characters and non-ASCII characters.
 * - JSON_UTF8:  Escapes control characters only.
 * The default value is JSON_UTF8.
 */
static enum json_encode encode = JSON_UTF8;

enum json_encode json_get_encode(void)
{
    return encode;
}

void json_set_encode(enum json_encode value)
{
    encode = value;
}

/**
 * According to IEEE 754-1985, double can represent numbers with maximum
 * accuracy of 17 digits after point. Add to it both minuses for mantissa
 * and period, point, e-char and 3 digits of period (8 bit), and you will
 * get exact 24 chars.
 */
#define MAX_DECIMALS 17
#define NUMBER_CHARS 24

#define print_number(buffer, ...) (size_t) \
    snprintf(buffer->text + buffer->length, NUMBER_CHARS + 1, __VA_ARGS__)

static char *buffer_print_integer(buffer_t *buffer, double value)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = print_number(buffer, "%.0f", value);

    buffer->length += length;
    return buffer->text;
}

static char *buffer_print_real(buffer_t *buffer, double value)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = print_number(buffer, "%.*g", MAX_DECIMALS, value);
    /* Dot followed by trailing zeros are removed when %g is used */
    int done = strspn(buffer->text + buffer->length, "-0123456789") != length;

    buffer->length += length;
    /* Write the fractional part if applicable */
    return done ? buffer->text : buffer_append(buffer, ".0");
}

static int buffer_print_string(buffer_t *buffer, const char *str)
{
    CHECK(buffer_putchr(buffer, '"'));

    const char *ptr = str;

    while (*str != '\0')
    {
        char esc = encode_esc(str);

        if (esc != '\0')
        {
            const char seq[] = {'\\', esc, '\0'};

            CHECK(buffer_attach(buffer, ptr, (size_t)(str - ptr)));
            CHECK(buffer_attach(buffer, seq, 2));
            ptr = ++str;
        }
        else if (is_cntrl(*str) || ((encode == JSON_ASCII) && !is_ascii(*str)))
        {
            char seq[sizeof("\\u0123")] = {'\0'};
            size_t length = encode_hex(str, seq);

            CHECK(buffer_attach(buffer, ptr, (size_t)(str - ptr)));
            CHECK(buffer_attach(buffer, seq, 6));
            str += length;
            ptr = str;
        }
        else
        {
            str++;
        }
    }
    CHECK(buffer_attach(buffer, ptr, (size_t)(str - ptr)));
    CHECK(buffer_putchr(buffer, '"'));
    return 1;
}

static int buffer_print_node(buffer_t *buffer, const json_t *node,
    unsigned short depth, unsigned char indent, unsigned char trailing_comma)
{
    for (unsigned i = 0; i < depth * indent; i++)
    {
        buffer_putchr(buffer, ' ');
    }
    if (node->key != NULL)
    {
        buffer_print_string(buffer, node->key);
        buffer_append(buffer, indent == 0 ? ":" : ": ");
    }
    switch (node->type)
    {
        case JSON_OBJECT:
            buffer_putchr(buffer, '{');
            break;
        case JSON_ARRAY:
            buffer_putchr(buffer, '[');
            break;
        case JSON_STRING:
            buffer_print_string(buffer, node->string);
            break;
        case JSON_INTEGER:
            buffer_print_integer(buffer, node->number);
            break;
        case JSON_REAL:
            buffer_print_real(buffer, node->number);
            break;
        case JSON_TRUE:
            buffer_append(buffer, "true");
            break;
        case JSON_FALSE:
            buffer_append(buffer, "false");
            break;
        case JSON_NULL:
            buffer_append(buffer, "null");
            break;
    }
    if (node->size == 0)
    {
        switch (node->type)
        {
            case JSON_OBJECT:
                buffer_putchr(buffer, '}');
                break;
            case JSON_ARRAY:
                buffer_putchr(buffer, ']');
                break;
        }
        if (trailing_comma)
        {
            buffer_putchr(buffer, ',');
        }
    }
    if (indent > 0)
    {
        buffer_putchr(buffer, '\n');
    }
    return !buffer->fail;
}

static int buffer_print_edge(buffer_t *buffer, const json_t *node,
    unsigned short depth, unsigned char indent, unsigned char trailing_comma)
{
    if (node->size != 0)
    {
        for (unsigned i = 0; i < depth * indent; i++)
        {
            buffer_putchr(buffer, ' ');
        }
        switch (node->type)
        {
            case JSON_OBJECT:
                buffer_putchr(buffer, '}');
                break;
            case JSON_ARRAY:
                buffer_putchr(buffer, ']');
                break;
        }
        if (trailing_comma)
        {
            buffer_putchr(buffer, ',');
        }
        if (indent > 0)
        {
            buffer_putchr(buffer, '\n');
        }
    }
    return !buffer->fail;
}

#define MAX_INDENT 8

typedef struct
{
    buffer_t *base;
    size_t max_length;
    unsigned char indent;
} json_buffer_t;

static int buffer_print_tree(const json_buffer_t *buffer, const json_t *node,
    unsigned short depth)
{
    for (unsigned i = 0;
        (i < node->size) && (buffer->base->length <= buffer->max_length);
        (i++))
    {
        unsigned char more = node->size > i + 1;

        CHECK(buffer_print_node(
            buffer->base, node->child[i], depth, buffer->indent, more));
        if (node->child[i]->size > 0)
        {
            CHECK(buffer_print_tree(buffer, node->child[i], depth + 1));
            CHECK(buffer_print_edge(
                buffer->base, node->child[i], depth, buffer->indent, more));
        }
    }
    return 1;
}

/**
 * Encodes a node into a provided buffer.
 * The cast from 'const json_t *' to 'json_t *' is needed to pack the children.
 * If the passed node IS a property, add parent and grandparent: [{key: value}]
 * If the passed node IS NOT a property, add parent: [value]
 */ 
static int buffer_encode(buffer_t *base, const json_t *node, size_t indent,
    size_t max_length)
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

        const json_buffer_t buffer =
        {
            .base = base,
            .max_length = max_length ? base->length + max_length : (size_t)-1,
            .indent = indent > MAX_INDENT ? MAX_INDENT : (unsigned char)indent
        };

        node = is_property ? &grandparent : &parent; 
        CHECK(buffer_print_tree(&buffer, node, 0));
        if (base->length > buffer.max_length)
        {
            buffer_adjust(base, buffer.max_length);
            buffer_append(base, indent ? "...\n" : "...");
        }
        return !base->fail;
    }
    return 0;
}

/* Serializes a JSON structure or a single node into a compact string */
char *json_encode(const json_t *node, size_t indent)
{
    buffer_t buffer = {0};

    if (buffer_encode(&buffer, node, indent, 0))
    {
        return buffer.text;
    }
    free(buffer.text);
    return NULL;
}

/* Serializes and limits length */
char *json_encode_max(const json_t *node, size_t indent, size_t max_length)
{
    buffer_t buffer = {0};

    if (buffer_encode(&buffer, node, indent, max_length))
    {
        return buffer.text;
    }
    free(buffer.text);
    return NULL;
}

/* Serializes into a provided buffer */
char *json_buffer_encode(buffer_t *buffer, const json_t *node, size_t indent)
{
    if (buffer_encode(buffer, node, indent, 0))
    {
        return buffer->text;
    }
    return NULL;
}

/* Serializes into a provided buffer and limits length */
char *json_buffer_encode_max(buffer_t *buffer, const json_t *node,
    size_t indent, size_t max_length)
{
    if (buffer_encode(buffer, node, indent, max_length))
    {
        return buffer->text;
    }
    return NULL;
}

/* Serializes without indentation */
char *json_stringify(const json_t *node)
{
    buffer_t buffer = {0};

    if (buffer_encode(&buffer, node, 0, 0))
    {
        return buffer.text;
    }
    free(buffer.text);
    return NULL;
}

#define write_file(buffer, file) \
    (fwrite(buffer.text, 1, buffer.length, file) == buffer.length)

/* Serializes into a file */
int json_write(const json_t *node, FILE *file, size_t indent)
{
    int rc = 0;

    if (file != NULL)
    {
        buffer_t buffer = {0};

        if (buffer_encode(&buffer, node, indent, 0))
        {
            rc = write_file(buffer, file);
        }
        free(buffer.text);
    }
    return rc;
}

/* Serializes into a file with a trailing newline */
int json_write_line(const json_t *node, FILE *file)
{
    int rc = 0;

    if (file != NULL)
    {
        buffer_t buffer = {0};

        if (buffer_encode(&buffer, node, 0, 0) &&
            buffer_putchr(&buffer, '\n'))
        {
            rc = write_file(buffer, file);
        }
        free(buffer.text);
    }
    return rc;
}

/* Serializes into a FILE given a path */
int json_write_file(const json_t *node, const char *path, size_t indent)
{
    FILE *file;
    int rc = 0;

    if ((node != NULL) && (path != NULL) && (file = fopen(path, "w")))
    {
        buffer_t buffer = {0};

        if (buffer_encode(&buffer, node, indent, 0))
        {
            rc = write_file(buffer, file);
        }
        free(buffer.text);
        fclose(file);
    }
    return rc;
}

/* Serializes and sends the result to stdout (2 spaces) */
int json_print(const json_t *node)
{
    return json_write(node, stdout, 2);
}

/* Returns an encoded json string */
char *json_quote(const char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    buffer_t buffer = {0};

    if (buffer_print_string(&buffer, str))
    {
        return buffer.text;
    }
    free(buffer.text);
    return NULL;
}

