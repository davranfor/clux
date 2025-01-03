/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "clib_unicode.h"
#include "json_private.h"
#include "json_buffer.h"

/* return 0 if buffer allocation fails */
#define CHECK(expr) do { if (!(expr)) return 0; } while (0)

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

#define buffer_write_number(buffer, ...) (size_t) \
    snprintf(buffer->text + buffer->length, NUMBER_CHARS + 1, __VA_ARGS__)

static char *buffer_write_integer(buffer_t *buffer, double value)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = buffer_write_number(buffer, "%.0f", value);

    buffer->length += length;
    return buffer->text;
}

static char *buffer_write_real(buffer_t *buffer, double value)
{
    if (buffer->size - buffer->length <= NUMBER_CHARS)
    {
        CHECK(buffer_resize(buffer, NUMBER_CHARS));
    }

    size_t length = buffer_write_number(buffer, "%.*g", MAX_DECIMALS, value);
    /* Dot followed by trailing zeros are removed when %g is used */
    int done = strspn(buffer->text + buffer->length, "-0123456789") != length;

    buffer->length += length;
    /* Write the fractional part if applicable */
    return done ? buffer->text : buffer_append(buffer, ".0");
}

static int buffer_write_string(buffer_t *buffer, const char *str)
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
        CHECK(buffer_putchr(buffer, ' '));
    }
    if (node->key != NULL)
    {
        CHECK(buffer_write_string(buffer, node->key));
        CHECK(buffer_append(buffer, indent == 0 ? ":" : ": "));
    }
    switch (node->type)
    {
        case JSON_OBJECT:
            CHECK(buffer_putchr(buffer, '{'));
            break;
        case JSON_ARRAY:
            CHECK(buffer_putchr(buffer, '['));
            break;
        case JSON_STRING:
            CHECK(buffer_write_string(buffer, node->string));
            break;
        case JSON_INTEGER:
            CHECK(buffer_write_integer(buffer, node->number));
            break;
        case JSON_REAL:
            CHECK(buffer_write_real(buffer, node->number));
            break;
        case JSON_TRUE:
            CHECK(buffer_append(buffer, "true"));
            break;
        case JSON_FALSE:
            CHECK(buffer_append(buffer, "false"));
            break;
        case JSON_NULL:
            CHECK(buffer_append(buffer, "null"));
            break;
    }
    if (node->size == 0)
    {
        switch (node->type)
        {
            case JSON_OBJECT:
                CHECK(buffer_putchr(buffer, '}'));
                break;
            case JSON_ARRAY:
                CHECK(buffer_putchr(buffer, ']'));
                break;
        }
        if (trailing_comma)
        {
            CHECK(buffer_putchr(buffer, ','));
        }
    }
    if (indent > 0)
    {
        CHECK(buffer_putchr(buffer, '\n'));
    }
    return 1;
}

static int buffer_print_edge(buffer_t *buffer, const json_t *node,
    unsigned short depth, unsigned char indent, unsigned char trailing_comma)
{
    if (node->size != 0)
    {
        for (unsigned i = 0; i < depth * indent; i++)
        {
            CHECK(buffer_putchr(buffer, ' '));
        }
        switch (node->type)
        {
            case JSON_OBJECT:
                CHECK(buffer_putchr(buffer, '}'));
                break;
            case JSON_ARRAY:
                CHECK(buffer_putchr(buffer, ']'));
                break;
        }
        if (trailing_comma)
        {
            CHECK(buffer_putchr(buffer, ','));
        }
        if (indent > 0)
        {
            CHECK(buffer_putchr(buffer, '\n'));
        }
    }
    return 1;
}

static int buffer_print_tree(buffer_t *buffer, const json_t *node,
    unsigned short depth, unsigned char indent)
{
    for (unsigned i = 0; i < node->size; i++)
    {
        unsigned char trailing_comma = node->size > i + 1;

        CHECK(buffer_print_node(
            buffer, node->child[i], depth, indent, trailing_comma));
        if (node->child[i]->size > 0)
        {
            CHECK(buffer_print_tree(
                buffer, node->child[i], depth + 1, indent));
            CHECK(buffer_print_edge(
                buffer, node->child[i], depth, indent, trailing_comma));
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
static int buffer_encode(buffer_t *buffer, const json_t *node, int indent)
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

        node = is_property ? &grandparent : &parent; 
        return buffer_print_tree(buffer, node, 0, (unsigned char)indent);
    }
    return 0;
}

/**
 * Serializes a JSON structure or a single node into a compact string without
 * applying any indentation or whitespace for formatting.
 * The output is suitable for scenarios where minimizing size is important,
 * such as network transmission or storage.
 * Returns a dynamically allocated string containing the JSON representation,
 * or NULL on failure.
 * The caller is responsible for freeing the returned string.
 */
char *json_stringify(const json_t *node)
{
    buffer_t buffer = {NULL, 0, 0};

    if (buffer_encode(&buffer, node, 0))
    {
        return buffer.text;
    }
    free(buffer.text);
    return NULL;
}

/* Serializes a JSON structure with indentation (truncated to 0 ... 8 spaces) */
char *json_encode(const json_t *node, int indent)
{
    buffer_t buffer = {NULL, 0, 0};

    if (buffer_encode(&buffer, node, indent < 0 ? 0 : indent > 8 ? 8 : indent))
    {
        return buffer.text;
    }
    free(buffer.text);
    return NULL;
}

/* Serializes a JSON structure into a provided buffer */
char *json_buffer_encode(buffer_t *buffer, const json_t *node, int indent)
{
    if (buffer_encode(buffer, node, indent < 0 ? 0 : indent > 8 ? 8 : indent))
    {
        return buffer->text;
    }
    return NULL;
}

/* Serializes a JSON structure into a file */
int json_write(const json_t *node, FILE *file, int indent)
{
    buffer_t buffer = {NULL, 0, 0};
    int rc = 0;

    if (file != NULL)
    {
        if (json_buffer_encode(&buffer, node, indent))
        {
            rc = fwrite(buffer.text, 1, buffer.length, file) == buffer.length;
            free(buffer.text);
        }
    }
    return rc;
}

/* Serializes a JSON structure into a file with a trailing newline */
int json_write_line(const json_t *node, FILE *file)
{
    buffer_t buffer = {NULL, 0, 0};
    int rc = 0;

    if (file != NULL)
    {
        if (json_buffer_encode(&buffer, node, 0) && buffer_putchr(&buffer, '\n'))
        {
            rc = fwrite(buffer.text, 1, buffer.length, file) == buffer.length;
        }
        free(buffer.text);
    }
    return rc;
}

/* Serializes a JSON structure into a FILE given a path */
int json_write_file(const json_t *node, const char *path, int indent)
{
    buffer_t buffer = {NULL, 0, 0};
    FILE *file;
    int rc = 0;

    if ((path != NULL) && (file = fopen(path, "w")))
    {
        if (json_buffer_encode(&buffer, node, indent))
        {
            rc = fwrite(buffer.text, 1, buffer.length, file) == buffer.length;
            free(buffer.text);
        }
        fclose(file);
    }
    return rc;
}

/* Serializes a JSON structure and sends the result to stdout (2 spaces) */
int json_print(const json_t *node)
{
    return json_write(node, stdout, 2);
}

