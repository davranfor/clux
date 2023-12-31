/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include "json_private.h"
#include "json_unicode.h"

static enum json_type token_type(int token)
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

static inline int is_token(int c)
{
    return (c == '{') || (c == '}')
        || (c == '[') || (c == ']')
        || (c == ':') || (c == ',')
        || (c == '\0');
}

/* scan() helpers */

static const char *scan_quoted(const char *str)
{
    while ((*str != '"') && !is_cntrl(*str))
    {
        if (*str == '\\')
        {
            if (is_esc(str + 1))
            {
                str += 2;
                continue;
            }
            if (is_ucn(str + 1))
            {
                str += 6;
                continue;
            }
            break;
        }
        str++;
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
    size_t size = (size_t)(end - str) + 1;
    char *buf = malloc(size);

    if (buf == NULL)
    {
        return NULL;
    }

    char *ptr = buf;

    while (str < end)
    {
        if (*str == '\\')
        {
            if (str[1] == 'u')
            {
                ptr += to_mbs(str + 2, ptr);
                str += 6;
            }
            else
            {
                ptr += to_esc(str + 1, ptr);
                str += 2;
            }
        }
        else
        {
            *ptr++ = *str++;
        }
    }
    *ptr = '\0';
    return buf;
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

    if ((end != right--) || isnan(number) || isinf(number))
    {
        return 0;
    }
    /* Skip sign */
    if (left[0] == '-')
    {
        left++;
    }
    /* Do not allow padding 0s */
    if ((left[0] == '0') && is_digit(left[1]))
    {
        return 0;
    }
    /* Must start and end with a digit */
    if (!is_digit(*left) || !is_digit(*right))
    {
        return 0;
    }
    /* Valid number */
    if (left + strspn(left, "0123456789") > right)
    {
        node->type = JSON_INTEGER;
    }
    else
    {
        node->type = JSON_NUMBER;
    }
    node->value.number = number;
    return 1;
}

static int try_set_true(json *node, const char *left, const char *right)
{
    if ((right - left == 4) && !strncmp(left, "true", 4))
    {
        node->type = JSON_BOOLEAN;
        node->value.number = 1;
        return 1;
    }
    return 0;
}

static int try_set_false(json *node, const char *left, const char *right)
{
    if ((right - left == 5) && !strncmp(left, "false", 5))
    {
        node->type = JSON_BOOLEAN;
        node->value.number = 0;
        return 1;
    }
    return 0;
}

static int try_set_null(json *node, const char *left, const char *right)
{
    if ((right - left == 4) && !strncmp(left, "null", 4))
    {
        node->type = JSON_NULL;
        node->value.number = 0;
        return 1;
    }
    return 0;
}

static int set_value(json *node, const char *left, const char *right)
{
    assert(right > left);
    assert(node->type == JSON_UNDEFINED);
    return try_set_string(node, left, right)
        || try_set_true  (node, left, right)
        || try_set_false (node, left, right)
        || try_set_null  (node, left, right)
        || try_set_number(node, left, right);
}

static json *create_node(void)
{
    return calloc(1, sizeof(struct json));
}

/* parse() helpers - node must exist */

static json *create_child(json *parent)
{
    json *child = calloc(1, sizeof(struct json));

    if (child != NULL)
    {
        child->parent = parent;
        parent->child = child;
    }
    return child;
}

static json *delete_child(json *parent)
{
    free(parent->child);
    parent->child = NULL;
    return parent;
}

static json *create_next(json *node)
{
    json *next = calloc(1, sizeof(struct json));

    if (next != NULL)
    {
        next->parent = node->parent;
        next->prev = node;
        node->next = next;
    }
    return next;
}

/* Parse document - returns an error position or NULL on success */
static const char *parse(json *node, const char *left)
{
    const char *right = NULL;
    const char *token;

    while (node != NULL)
    {
        if (!(token = scan(&left, &right)))
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
                if (json_is_object(node->parent) && (node->name == NULL))
                {
                    return token;
                }
                node->type = token_type(*token);
                node = create_child(node);
                break;
            case ':':
                if (left == token)
                {
                    return left;
                }
                /* Only object properties can have a name */
                if (!json_is_object(node->parent) || (node->name != NULL))
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
                    if (json_is_object(node->parent) && (node->name == NULL))
                    {
                        return left;
                    }
                    if (!set_value(node, left, right))
                    {
                        return left;
                    }
                }
                else
                {
                    if (left != token)
                    {
                        return left;
                    }
                }
                node = create_next(node);
                break;
            case ']':
            case '}':
                if (json_type(node->parent) != token_type(*token))
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
                            node = delete_child(node->parent);
                            break;
                        }
                        return left;
                    }
                    if (json_is_object(node->parent) && (node->name == NULL))
                    {
                        return left;
                    }
                    if (!set_value(node, left, right))
                    {
                        return left;
                    }
                }
                else
                {
                    if (left != token)
                    {
                        return left;
                    }
                }
                node = node->parent;
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
                else
                {
                    if (left != token)
                    {
                        return left;
                    }
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

static char *read_file(FILE *file, size_t size)
{
    char *str = malloc(size + 1);

    if (str != NULL)
    {
        if (fread(str, 1, size, file) == size)
        {
            str[size] = '\0';
        }
        else
        {
            free(str);
            str = NULL;
        }
    }
    return str;
}

static char *read_file_from_path(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }
    
    FILE *file = fopen(path, "rb");

    if (file == NULL)
    {
        return NULL;
    }

    char *str = NULL;

    if (fseek(file, 0L, SEEK_END) != -1)
    {
        long size = ftell(file);

        if ((size != -1) && (fseek(file, 0L, SEEK_SET) != -1))
        {
            str = read_file(file, (size_t)size);
        }
    }
    fclose(file);
    return str;
}

json *json_parse_file(const char *path, json_error *error)
{
    char *str = read_file_from_path(path);
    json *node = json_parse(str, error);

    free(str);
    return node;
}

void json_print_error(const json_error *error)
{
    if ((error == NULL) || (error->line == 0))
    {
        fprintf(stderr, "json - %s\n", strerror(errno));
    }
    else
    {
        fprintf(stderr, "json: Error at line %d, column %d\n",
            error->line,
            error->column
        );
    }
}

