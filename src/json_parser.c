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
#include "json_writer.h"
#include "json_parser.h"

static unsigned short max_depth = (unsigned short)-1;

void json_parser_set_max_depth(unsigned short depth)
{
    if (depth == 0)
    {
        max_depth = (unsigned short)-1;
    }
    else
    {
        max_depth = depth;
    }
}

unsigned short json_parser_get_max_depth(void)
{
    return max_depth;
}

static void clear_error(json_error_t *error)
{
    if (error != NULL)
    {
        error->line = error->column = 0;
    }
}

static void set_error(json_error_t *error, const char *str, const char *end)
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

static json_t *append(json_t *parent, json_t *child)
{
    if (child == NULL)
    {
        return NULL;
    }

    unsigned size = next_size(parent->size);

    if (size > parent->size) 
    {
        json_t **childs = realloc(parent->child, sizeof(*childs) * size);

        if (childs == NULL)
        {
            return NULL;
        }
        parent->child = childs;
    }
    parent->child[parent->size++] = child;
    child->packed = 1;
    return child;    
}

static json_t *new_node(unsigned char type)
{
    json_t *node = calloc(1, sizeof *node);

    if (node != NULL)
    {
        node->type = type;
    }
    return node;
}

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

static const char *skip_whitespaces(const char *str)
{
    while (is_space(*str))
    {
        str++;
    }
    return str;
}

static const char *scan_string(const char *str)
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

static json_t *parse(const char **, unsigned short);

static char *parse_key(const char **str)
{
    if (**str != '"')
    {
        return NULL;
    }

    const char *key = ++*str; 
    const char *end = scan_string(*str);

    if (*end != '"')
    {
        *str = end;
        return NULL;
    }
    *str = skip_whitespaces(end + 1);
    if (**str != ':')
    {
        return NULL;
    }
    *str = skip_whitespaces(++*str);
    return new_string(key, end);
}

static json_t *parse_object(const char **str, unsigned short depth)
{
    json_t *parent = new_node(JSON_OBJECT);

    if (parent == NULL)
    {
        return NULL;
    }

    *str = skip_whitespaces(++*str);

    int trailing_comma = 0;

    while (**str != '}')
    {
        if (((parent->size > 0) && (trailing_comma == 0)) || (depth >= max_depth))
        {
            json_delete(parent);
            return NULL;
        }

        char *key = parse_key(str);

        if (key == NULL)
        {
            json_delete(parent);
            return NULL;
        }

        json_t *child = parse(str, depth + 1);

        if (!append(parent, child))
        {
            json_delete(parent);
            json_delete(child);
            free(key);
            return NULL;
        }
        child->key = key;
        if (**str == ',')
        {
            *str = skip_whitespaces(++*str);
            trailing_comma = 1;
        }
        else
        {
            trailing_comma = 0;
        }        
    }
    if (trailing_comma != 0)
    {
        json_delete(parent);
        return NULL;
    }
    *str = skip_whitespaces(++*str);
    return parent; 
}

static json_t *parse_array(const char **str, unsigned short depth)
{
    json_t *parent = new_node(JSON_ARRAY);

    if (parent == NULL)
    {
        return NULL;
    }

    *str = skip_whitespaces(++*str);

    int trailing_comma = 0;

    while (**str != ']')
    {
        if (((parent->size > 0) && (trailing_comma == 0)) || (depth >= max_depth))
        {
            json_delete(parent);
            return NULL;
        }

        json_t *child = parse(str, depth + 1);

        if (!append(parent, child))
        {
            json_delete(parent);
            json_delete(child);
            return NULL;
        }
        if (**str == ',')
        {
            *str = skip_whitespaces(++*str);
            trailing_comma = 1;
        }
        else
        {
            trailing_comma = 0;
        }        
    }
    if (trailing_comma != 0)
    {
        json_delete(parent);
        return NULL;
    }
    *str = skip_whitespaces(++*str);
    return parent; 
}

static json_t *parse_string(const char **str)
{
    const char *end = scan_string(++*str);

    if (*end != '"')
    {
        *str = end;
        return NULL;
    }

    char *string = new_string(*str, end);

    if (string == NULL)
    {
        return NULL;
    }

    json_t *node = new_node(JSON_STRING);

    if (node == NULL)
    {
        free(string);
        return NULL;
    }
    node->string = string;
    *str = skip_whitespaces(end + 1);
    return node;
}

static json_t *parse_number(const char **str)
{
    char *end;
    double number = strtod(*str, &end);

    if (errno == ERANGE)
    {
        errno = 0;
        return NULL;
    }
    if (isnan(number) || isinf(number))
    {
        return NULL;
    }

    json_t *node = new_node(JSON_UNDEFINED);

    if (node == NULL)
    {
        return NULL;
    }
    node->number = number;
    node->type = (*str + strspn(*str, "-0123456789") >= end)
        ? is_safe_integer(number) ? JSON_INTEGER : JSON_REAL
        : JSON_REAL;
    *str = end;
    return node; 
}

static json_t *parse_true(const char **str)
{
    if (strncmp(*str, "true", 4))
    {
        return NULL;
    }
    *str = skip_whitespaces(*str + 4);
    return new_node(JSON_TRUE);
}

static json_t *parse_false(const char **str)
{
    if (strncmp(*str, "false", 5))
    {
        return NULL;
    }
    *str = skip_whitespaces(*str + 5);
    return new_node(JSON_FALSE);
}

static json_t *parse_null(const char **str)
{
    if (strncmp(*str, "null", 4))
    {
        return NULL;
    }
    *str = skip_whitespaces(*str + 4);
    return new_node(JSON_NULL);
}

static json_t *parse(const char **str, unsigned short depth)
{
    switch (**str)
    {
        case '{':
            return parse_object(str, depth);
        case '[':
            return parse_array(str, depth);
        case '"':
            return parse_string(str);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number(str);
        case 't':
            return parse_true(str);
        case 'f':
            return parse_false(str);
        case 'n':
            return parse_null(str);
        default:
            return NULL;
    }
}

json_t *json_parse(const char *str, json_error_t *error)
{
    clear_error(error);

    if (str == NULL)
    {
        return NULL;
    }

    const char *end = skip_whitespaces(str);
    json_t *node = parse(&end, 0);

    if ((node == NULL) || (*end != '\0'))
    {
        set_error(error, str, end);
        json_delete(node);
        return NULL;
    }
    return node;
}

json_t *json_parse_file(const char *path, json_error_t *error)
{
    char *str = file_read(path);
    json_t *node = json_parse(str, error);

    free(str);
    return node;
}

void json_print_error(const json_error_t *error)
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

