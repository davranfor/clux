/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <string.h>
#include "json_private.h"

/**
 * Some examples:
 *   "integer"
 *   "unique integer"
 *   "array of integers"
 *   "array of unique integers"
 *   "array of optional integers"
 *   "array of optional unique integers"
 *   "array of unique optional integers"
 *   "unique array of unique optional integers"
 */

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
    {{"real",     "reals"    }, json_is_real    },
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

enum {MAX_TOKENS = 6};

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
    int unique, iterable;
    struct { int optional, unique; } childs;
    int (*func[2])(const json *);
};

static int compare(const struct token *token, const char *text)
{
    return !strncmp(text, token->string, token->length)
        && !text[token->length];
}

static int set_function(struct query *query, const struct token *token)
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

static int set_unique(struct query *query, const struct token *token)
{
    return compare(token, "unique") && (query->unique = 1);
}

static int set_iterable(struct query *query, const struct token *token)
{
    return compare(token, "of") && (query->iterable = 1);
}

static int set_settings(struct query *query, const struct token *token)
{
    if (!query->childs.optional && compare(token, "optional"))
    {
        return (query->childs.optional = 1);
    }
    if (!query->childs.unique && compare(token, "unique"))
    {
        return (query->childs.unique = 1);
    }
    return 0;    
}

static int set_query(struct query *query, const struct token *tokens,
    size_t size)
{
    if ((size > 0) && set_unique(query, &tokens[0]))
    {
        tokens++;
        size--;
    }
    return ((size > 0) && set_function(query, &tokens[0]))
        && ((size < 2) || set_iterable(query, &tokens[1]))
        && ((size < 4) || set_settings(query, &tokens[2]))
        && ((size < 5) || set_settings(query, &tokens[3]))
        && ((size < 2) || set_function(query, &tokens[size - 1]));
}

static int has_simple_childs(const json *node, int (*func)(const json *))
{
    for (node = node->head; func(node) != 0; node = node->next);
    return node == NULL;
}

static int has_unique_childs(const json *node, int (*func)(const json *))
{
    const json *head = node->head;

    for (node = head; func(node) != 0; node = node->next)
    {
        for (const json *item = head; item != node; item = item->next)
        {
            if (json_equal(node, item))
            {
                return 0;
            }
        }
    }
    return node == NULL;
}

static int run_query(struct query *query, const json *node)
{
    if (!query->func[0](node) || (query->unique && !json_is_unique(node)))
    {
        return 0;
    }
    if (query->iterable)
    {
        return node->head
            ? query->childs.unique
                ? has_unique_childs(node, query->func[1])
                : has_simple_childs(node, query->func[1])
            : query->childs.optional && json_is_iterable(node);
    }
    return 1;
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

    return set_query(&query, tokens, size)
        && run_query(&query, node);
}

int json_is_unique(const json *node)
{
    if (node == NULL)
    {
        return 0;
    }
    for (const json *item = node->prev; item != NULL; item = item->prev)
    {
        if (json_equal(node, item))
        {
            return 0;
        }
    }
    for (const json *item = node->next; item != NULL; item = item->next)
    {
        if (json_equal(node, item))
        {
            return 0;
        }
    }
    return 1;
}

