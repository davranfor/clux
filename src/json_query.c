/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include "json_private.h"

#define MAX_TOKENS 5

static const struct
{
    const char *text[2];
    int (*func)(const json *);
}
map[] =
{
    {{"iterable", "iterables"}, json_is_iterable},
    {{"object",   "objects"  }, json_is_object  },
    {{"array",    "arrays"   }, json_is_array   },
    {{"scalar",   "scalars"  }, json_is_scalar  },
    {{"string",   "strings"  }, json_is_string  },
    {{"integer",  "integers" }, json_is_integer },
    {{"unsigned", "unsigneds"}, json_is_unsigned},
    {{"double",   "doubles"  }, json_is_double  },
    {{"number",   "numbers"  }, json_is_number  },
    {{"boolean",  "booleans" }, json_is_boolean },
    {{"null",     "nulls"    }, json_is_null    },
    {{"item",     "items"    }, json_is_any     },
};

struct token
{
    const char *string;
    size_t length;
};

static size_t set_tokens(struct token *tokens, const char *text)
{
    const char *spaces = " \t\r\n\f\v";
    size_t size = 0;

    for (size_t token = 0; token <= MAX_TOKENS; token++)
    {
        const char *start = text + strspn(text, spaces);

        text = start + strcspn(start, spaces);
        if (text == start)
        {
            return size;
        }
        if (size < MAX_TOKENS)
        {
            tokens[size].string = start;
            tokens[size].length = (size_t)(text - start);
            size++;
        }
    }
    return 0;
}

struct query
{
    int (*func[2])(const json *);
    int optional, unique;
};

static int compare(struct token *token, const char *text)
{
    return !strncmp(text, token->string, token->length)
        && !text[token->length];
}

static int set_func(struct query *query, struct token *token)
{
    size_t words = sizeof map / sizeof map[0];
    int id = query->func[0] == NULL ? 0 : 1;

    for (size_t iter = 0; iter < words; iter++)
    {
        if (compare(token, map[iter].text[id]))
        {
            query->func[id] = map[iter].func;
            return 1;
        }
    }
    return 0;
}

static int set_flag(struct query *query, struct token *token)
{
    if ((query->optional == 0) && compare(token, "optional"))
    {
        query->optional = 1;
        return 1;
    }
    if ((query->unique == 0) && compare(token, "unique"))
    {
        query->unique = 1;
        return 1;
    }
    return 0;    
}

static int set_query(struct query *query, struct token *tokens, size_t size)
{
    return ((size > 0) && set_func(query, &tokens[0]))
        && ((size < 2) || compare(&tokens[1], "of"))
        && ((size < 4) || set_flag(query, &tokens[2]))
        && ((size < 5) || set_flag(query, &tokens[3]))
        && ((size < 2) || set_func(query, &tokens[size - 1]));
}

static int is_common(const json *node, int (*func)(const json *))
{
    while (func(node) != 0)
    {
        node = node->next;
    }
    return node == NULL;
}

static int is_unique(const json *node, int (*func)(const json *))
{
    const json *head = node;

    while (func(node) != 0)
    {
        for (const json *item = head; item != node; item = item->next)
        {
            if (json_equal(node, item))
            {
                return 0;
            }
        }
        node = node->next;
    }
    return node == NULL;
}

int json_is(const json *node, const char *text)
{
    if ((node == NULL) || (text == NULL))
    {
        return 0;
    }

    struct token tokens[MAX_TOKENS] = {0};
    size_t size = set_tokens(tokens, text);
    struct query query = {0};

    if (!set_query(&query, tokens, size))
    {
        return 0;
    }

    int rc = query.func[0](node);

    if (rc && query.func[1])
    {
        return node->child
            ? query.unique
                ? is_unique(node->child, query.func[1])
                : is_common(node->child, query.func[1])
            : query.optional && json_is_iterable(node);
    }
    return rc;
}

