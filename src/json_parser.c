/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include "clib_math.h"
#include "clib_stream.h"
#include "clib_unicode.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_writer.h"
#include "json_parser.h"

static inline enum json_type token_type(int token)
{
    switch (token)
    {
        default : return JSON_UNDEFINED;
        case '{':
        case '}': return JSON_OBJECT;
        case '[':
        case ']': return JSON_ARRAY;
    }
}

static inline int is_token(int token)
{
    switch (token)
    {
        case '{':
        case '}':
        case '[':
        case ']':
        case ':':
        case ',':
        case  0 : return 1;
        default : return 0;
    }
}

/* scan() helpers */

static const char *scan_quoted(const char *str)
{
    while ((*str != '\"') && !is_cntrl(*str))
    {
        if (*str != '\\')
        {
            str += 1;
        }
        else if (is_esc(str + 1))
        {
            str += 2;
        }
        else if (is_hex(str + 1))
        {
            str += 6;
        }
        else
        {
            break;
        }
    }
    return str;
}

static const char *scan_unquoted(const char *str)
{
    while (!is_space(*str) && !is_token(*str))
    {
        str++;
    }
    return str;
}

/* Returns a pointer to the next element or NULL on fail */
static const char *scan(const char **left, const char **right)
{
    const char *str = *left;

    /* Skip leading spaces */
    while (is_space(*str))
    {
        str++;
    }
    /* Adjust pointers to token */
    *left = *right = str;
    /* Return on first token */
    if (is_token(*str))
    {
        return str;
    }
    /* Handle name or string scalar */
    if (*str == '"')
    {
        str = scan_quoted(str + 1);
        if (*str != '"')
        {
            goto error;
        }
        *right = str++;
    }
    else // ... handle other scalars
    {
        str = scan_unquoted(str + 1);
        *right = str;
    }
    /* Skip trailing spaces */
    while (is_space(*str))
    {
        str++;
    }
    if (is_token(*str))
    {
        return str;
    }
error:
    *left = str;
    return NULL;
}

/* Allocates space for a name or a string value escaping special characters */
static char *new_string(const char *str, const char *end)
{
    char *text = malloc((size_t)(end - str) + 1);

    if (text == NULL)
    {
        return NULL;
    }

    char *ptr = text;

    while (str < end)
    {
        if (str[0] != '\\')
        {
            *ptr++ = *str;
            str += 1;
        }
        else if (str[1] != 'u')
        {
            *ptr++ = decode_esc(str + 1);
            str += 2;
        }
        else
        {
            ptr += decode_hex(str + 2, ptr);
            str += 6;
        }
    }
    *ptr = '\0';
    return text;
}

static char *set_name(json *node, const char *left, const char *right)
{
    assert(right > left);
    if ((*left++ == '"') && (*right == '"'))
    {
        node->name = new_string(left, right);
        return node->name;
    }
    return NULL;
}

static int try_set_string(json *node, const char *left, const char *right)
{
    if ((*left++ == '"') && (*right == '"'))
    {
        node->value.string = new_string(left, right);
        if (node->value.string != NULL)
        {
            node->type = JSON_STRING;
            return 1;
        }
    }
    return 0;
}

static int try_set_number(json *node, const char *left, const char *right)
{
    char *end;
    double number = strtod(left, &end);

    if ((end == right) && !isnan(number) && !isinf(number))
    {
        node->value.number = number;
        node->type = (left + strspn(left, "+-xX0123456789") >= right)
            ? is_safe_integer(number) ? JSON_INTEGER : JSON_REAL
            : JSON_REAL;
        return 1;
    }
    return 0;
}

static int try_set_true(json *node, const char *left, const char *right)
{
    if ((right - left == 4) && !strncmp(left, "true", 4))
    {
        node->value.number = 1;
        node->type = JSON_BOOLEAN;
        return 1;
    }
    return 0;
}

static int try_set_false(json *node, const char *left, const char *right)
{
    if ((right - left == 5) && !strncmp(left, "false", 5))
    {
        node->value.number = 0;
        node->type = JSON_BOOLEAN;
        return 1;
    }
    return 0;
}

static int try_set_null(json *node, const char *left, const char *right)
{
    if ((right - left == 4) && !strncmp(left, "null", 4))
    {
        node->value.number = 0;
        node->type = JSON_NULL;
        return 1;
    }
    return 0;
}

static int set_value(json *node, const char *left, const char *right)
{
    assert(right > left);
    assert(node->type == JSON_UNDEFINED);
    return try_set_string(node, left, right)
        || try_set_true(node, left, right)
        || try_set_false(node, left, right)
        || try_set_null(node, left, right)
        || try_set_number(node, left, right);
}

static json *create_node(void)
{
    return calloc(1, sizeof(struct json));
}

/* parse() helpers - node must exist */

static json *create_head(json *parent)
{
    json *head = calloc(1, sizeof(struct json));

    if (head != NULL)
    {
        head->parent = parent;
        parent->head = head;
        parent->tail = head;
        parent->size = 1;
    }
    return head;
}

static json *delete_head(json *parent)
{
    free(parent->head);
    parent->head = NULL;
    parent->tail = NULL;
    parent->size = 0;
    return parent;
}

static json *create_next(json *node)
{
    json *next = calloc(1, sizeof(struct json));

    if (next != NULL)
    {
        next->parent = node->parent;
        next->parent->tail = next;
        next->parent->size++;
        next->prev = node;
        node->next = next;
    }
    return next;
}

/* Parse document - returns an error position or NULL on success */
static const char *parse(json *node, const char *left)
{
    enum json_type parent = JSON_UNDEFINED;
    const char *right = NULL;

    while (node != NULL)
    {
        const char *token = scan(&left, &right);

        if (token == NULL)
        {
            return left;
        }
        switch (*token)
        {
            case '{':
            case '[':
                /* Commas between iterables are required: [[] []] */
                if (node->type != JSON_UNDEFINED)
                {
                    return left;
                }
                /* Contents before iterables are not allowed: 1[] */
                if (left != token)
                {
                    return left;
                }
                /* Object properties must have a name */
                if ((parent == JSON_OBJECT) && (node->name == NULL))
                {
                    return token;
                }
                parent = node->type = token_type(*token);
                node = create_head(node);
                break;
            case ':':
                if (left == token)
                {
                    return left;
                }
                /* Only object properties can have a name */
                if ((parent != JSON_OBJECT) || (node->name != NULL))
                {
                    return token;
                }
                if (!set_name(node, left, right))
                {
                    return left;
                }
                break;
            case ',':
                if (node->parent == NULL)
                {
                    return token;
                }
                if (node->type == JSON_UNDEFINED)
                {
                    if (left == token)
                    {
                        return left;
                    }
                    if ((parent == JSON_OBJECT) && (node->name == NULL))
                    {
                        return left;
                    }
                    if (!set_value(node, left, right))
                    {
                        return left;
                    }
                }
                else if (left != token)
                {
                    return left;
                }
                node = create_next(node);
                break;
            case ']':
            case '}':
                if (parent != token_type(*token))
                {
                    return token;
                }
                if (node->type == JSON_UNDEFINED)
                {
                    if (left == token)
                    {
                        /* Remove empty iterable: {} or [] */
                        if ((node->prev == NULL) && (node->name == NULL))
                        {
                            node = delete_head(node->parent);
                            parent = json_type(node->parent);
                            break;
                        }
                        return left;
                    }
                    if ((parent == JSON_OBJECT) && (node->name == NULL))
                    {
                        return left;
                    }
                    if (!set_value(node, left, right))
                    {
                        return left;
                    }
                }
                else if (left != token)
                {
                    return left;
                }
                node = node->parent;
                parent = json_type(node->parent);
                break;
            case '\0':
                /* Bad closed document */
                if (node->parent != NULL)
                {
                    return left;
                }
                if (node->type == JSON_UNDEFINED)
                {
                    if (left == token)
                    {
                        return left;
                    }
                    if (!set_value(node, left, right))
                    {
                        return left;
                    }
                }
                else if (left != token)
                {
                    return left;
                }
                /* Correct document */
                return NULL;
        }
        /* Keep going ... */
        left = token + 1;
    }
    return left;
}

static void clear_error(json_error *error)
{
    if (error != NULL)
    {
        error->line = error->column = 0;
    }
}

static void set_error(json_error *error, const char *str, const char *end)
{
    if (error != NULL)
    {
        error->line = error->column = 1;
        for (; str < end; str++)
        {
            if (*str == '\n')
            {
                error->line++;
                error->column = 1;
            }
            else if (is_utf8(*str))
            {
                error->column++;
            }
        }
    }
}

json *json_parse(const char *str, json_error *error)
{
    clear_error(error);
    if (str == NULL)
    {
        return NULL;
    }

    json *node = create_node();

    if (node != NULL)
    {
        const char *end = parse(node, str);

        if (end != NULL)
        {
            set_error(error, str, end);
            json_free(node);
            return NULL;
        }
    }
    return node;
}

json *json_parse_file(const char *path, json_error *error)
{
    char *str = file_read(path);
    json *node = json_parse(str, error);

    free(str);
    return node;
}

void json_print_error(const json_error *error)
{
    if ((error == NULL) || (error->line == 0))
    {
        fprintf(stderr, "json: %s\n", strerror(errno));
    }
    else
    {
        fprintf(stderr, "json: Error at %d,%d\n", error->line, error->column);
    }
}

