/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "clib_math.h"
#include "clib_check.h"
#include "clib_unicode.h"
#include "json_private.h"
#include "json_buffer.h"

/**
 * The 'encoding' static global variable can take the following values:
 * - JSON_ASCII: Escapes control characters and non-ASCII characters.
 * - JSON_UTF8:  Escapes control characters only.
 * The default value is JSON_UTF8.
 */
static enum json_encoding encoding = JSON_UTF8;

enum json_encoding json_get_encoding(void)
{
    return encoding;
}

void json_set_encoding(enum json_encoding mode)
{
    encoding = mode;
}

/**
 * According to IEEE 754-1985, double can represent numbers with maximum
 * accuracy of 17 digits after point. Add to it both minuses for mantissa
 * and period, point, e-char and 3 digits of period (8 bit), and you will
 * get exact 24 chars.
 */
#define MAX_DECIMALS 17
#define NUMBER_CHARS 24

#define write_number(buffer, ...) (size_t) \
    snprintf(buffer->text + buffer->length, NUMBER_CHARS + 1, __VA_ARGS__)

static char *write_integer(buffer_t *buffer, double number)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = write_number(buffer, "%.0f", number);

    if (length > NUMBER_CHARS)
    {
        buffer_set_error(buffer, BUFFER_ERROR_FORMAT);
        return NULL;
    }
    buffer->length += length;
    return buffer->text;
}

static char *write_real(buffer_t *buffer, double number)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = write_number(buffer, "%.*g", MAX_DECIMALS, number);

    if (length > NUMBER_CHARS)
    {
        buffer_set_error(buffer, BUFFER_ERROR_FORMAT);
        return NULL;
    }

    /* Dot followed by trailing zeros are removed when %g is used */
    int done = strspn(buffer->text + buffer->length, "-0123456789") != length;

    buffer->length += length;
    /* Write the fractional part if applicable */
    return done ? buffer->text : buffer_write(buffer, ".0");
}

static char *write_string(buffer_t *buffer, const char *str)
{
    CHECK(buffer_put(buffer, '"'));

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
        else if (is_cntrl(*str) || ((encoding == JSON_ASCII) && !is_ascii(*str)))
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
    return buffer_put(buffer, '"');
}

static int encode_node(buffer_t *buffer, const json_t *node,
    unsigned short depth, unsigned char indent,
    unsigned char trailing_comma)
{
    size_t spaces = depth * indent;

    if (spaces > 0)
    {
        buffer_repeat(buffer, ' ', spaces);
    }
    if (node->key != NULL)
    {
        write_string(buffer, node->key);
        buffer_write(buffer, indent == 0 ? ":" : ": ");
    }
    switch (node->type)
    {
        case JSON_OBJECT:
            buffer_put(buffer, '{');
            break;
        case JSON_ARRAY:
            buffer_put(buffer, '[');
            break;
        case JSON_STRING:
            write_string(buffer, node->string);
            break;
        case JSON_INTEGER:
            write_integer(buffer, node->number);
            break;
        case JSON_REAL:
            write_real(buffer, node->number);
            break;
        case JSON_TRUE:
            buffer_write(buffer, "true");
            break;
        case JSON_FALSE:
            buffer_write(buffer, "false");
            break;
        case JSON_NULL:
            buffer_write(buffer, "null");
            break;
    }
    if (node->size == 0)
    {
        switch (node->type)
        {
            case JSON_OBJECT:
                buffer_put(buffer, '}');
                break;
            case JSON_ARRAY:
                buffer_put(buffer, ']');
                break;
        }
        if (trailing_comma)
        {
            buffer_put(buffer, ',');
        }
    }
    if (indent > 0)
    {
        buffer_put(buffer, '\n');
    }
    return buffer->text != NULL;
}

static int encode_edge(buffer_t *buffer, const json_t *node,
    unsigned short depth, unsigned char indent,
    unsigned char trailing_comma)
{
    size_t spaces = depth * indent;

    if (spaces > 0)
    {
        buffer_repeat(buffer, ' ', spaces);
    }
    switch (node->type)
    {
        case JSON_OBJECT:
            buffer_put(buffer, '}');
            break;
        case JSON_ARRAY:
            buffer_put(buffer, ']');
            break;
    }
    if (trailing_comma)
    {
        buffer_put(buffer, ',');
    }
    if (indent > 0)
    {
        buffer_put(buffer, '\n');
    }
    return buffer->text != NULL;
}

#define MAX_INDENT 8

typedef struct
{
    buffer_t *base;
    size_t max_length;
    unsigned char indent;
} json_buffer_t;

static int encode_tree(const json_buffer_t *buffer, const json_t *node, unsigned short depth)
{
    for (unsigned i = 0; (i < node->size) && (buffer->base->length <= buffer->max_length); i++)
    {
        unsigned char more = node->size > i + 1;

        CHECK(encode_node(buffer->base, node->child[i], depth, buffer->indent, more));
        if (node->child[i]->size > 0)
        {
            CHECK(encode_tree(buffer, node->child[i], depth + 1));
            CHECK(encode_edge(buffer->base, node->child[i], depth, buffer->indent, more));
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
static char *buffer_encode(buffer_t *base, const json_t *node, size_t indent,
    size_t max_length)
{
    if (node == NULL)
    {
        return NULL;
    }

    const json_t parent =
    {
        .child = (json_t *[]){json_cast(node)},
        .size = 1,
        .type = node->key ? JSON_OBJECT : JSON_ARRAY
    };
    const json_t grandparent =
    {
         .child = (json_t *[]){json_cast(&parent)},
         .size = 1,
         .type = JSON_ARRAY
    };
    const json_buffer_t buffer =
    {
        .base = base,
        .max_length = max_length ? base->length + max_length : (size_t)-1,
        .indent = indent > MAX_INDENT ? MAX_INDENT : (unsigned char)indent
    };

    CHECK(encode_tree(&buffer, node->key ? &grandparent : &parent, 0));
    if (base->length > buffer.max_length)
    {
        buffer_set_length(base, buffer.max_length);
        buffer_write(base, indent ? "...\n" : "...");
    }
    return base->text;
}

/* Serializes a JSON structure or a single node into a compact string */
char *json_encode(const json_t *node, size_t indent)
{
    buffer_t buffer = {0};

    return buffer_encode(&buffer, node, indent, 0);
}

/* Serializes and limits length */
char *json_encode_max(const json_t *node, size_t indent, size_t max_length)
{
    buffer_t buffer = {0};

    return buffer_encode(&buffer, node, indent, max_length);
}

/* Serializes into a provided buffer */
char *json_buffer_encode(buffer_t *buffer, const json_t *node, size_t indent)
{
    if (buffer && buffer_encode(buffer, node, indent, 0))
    {
        return buffer->text;
    }
    return NULL;
}

/* Serializes into a provided buffer and limits length */
char *json_buffer_encode_max(buffer_t *buffer, const json_t *node, size_t indent,
    size_t max_length)
{
    if (buffer && buffer_encode(buffer, node, indent, max_length))
    {
        return buffer->text;
    }
    return NULL;
}

/* Serializes without indentation */
char *json_stringify(const json_t *node)
{
    buffer_t buffer = {0};

    return buffer_encode(&buffer, node, 0, 0);
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

        if (buffer_encode(&buffer, node, 0, 0) && buffer_put(&buffer, '\n'))
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

    return write_string(&buffer, str);
}

/* Returns an encoded json string and limits length */
char *json_quote_max(const char *str, size_t max_length)
{
    if (str == NULL)
    {
        return NULL;
    }

    buffer_t buffer = {0};

    if (write_string(&buffer, str))
    {
        if ((max_length > 0) && (buffer.length - 2 > max_length))
        {
            buffer_set_length(&buffer, max_length + 1);
            buffer_write(&buffer, "...\"");
        }
    }
    return buffer.text;
}

/* Encodes a json string into a provided buffer */
char *json_buffer_quote(buffer_t *buffer, const char *str)
{
    if ((buffer == NULL) || (str == NULL))
    {
        return NULL;
    }
    return write_string(buffer, str);
}

/* Encodes a json string into a provided buffer and limits length */
char *json_buffer_quote_max(buffer_t *buffer, const char *str, size_t max_length)
{
    if ((buffer == NULL) || (str == NULL))
    {
        return NULL;
    }
    if (max_length > 0)
    {
        max_length += buffer->length;
    }
    else
    {
        max_length = (size_t)-1;
    }
    if (write_string(buffer, str))
    {
        if (buffer->length - 2 > max_length)
        {
            buffer_set_length(buffer, max_length + 1);
            buffer_write(buffer, "...\"");
        }
    }
    return buffer->text;
}

/* Returns an encoded json string from a number */
char *json_convert(double number, enum json_type type)
{
    buffer_t buffer = {0};

    if ((type == JSON_INTEGER) && IS_SAFE_INTEGER(number))
    {
        return write_integer(&buffer, number);
    }
    else
    {
        return write_real(&buffer, number);
    }
}

/* Encodes a number as json string into a provided buffer */
char *json_buffer_convert(buffer_t *buffer, double number, enum json_type type)
{
    if (buffer == NULL)
    {
        return NULL;
    }
    if ((type == JSON_INTEGER) && IS_SAFE_INTEGER(number))
    {
        return write_integer(buffer, number);
    }
    else
    {
        return write_real(buffer, number);
    }
}

