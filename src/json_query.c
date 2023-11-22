/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include "json_private.h"
#include "json_unicode.h"

#define MAX_WORDS 5

typedef int (*func_callback)(const json *);

struct
{
    const char *word[2];
    func_callback func;
}
static map[] =
{
    {{"item",     "items"    }, json_is_any     },
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
};

struct word
{
    const char *string;
    size_t length;
};

static size_t set_words(struct word *words, const char *text)
{
    size_t size = 0;

    for (size_t iter = 0; iter <= MAX_WORDS; iter++) 
    {
        const char *start;

        while (is_space(*text))
        {
            text++;
        }
        start = text;
        while (!is_space(*text) && (*text != '\0'))
        {
            text++;
        }
        if (text == start)
        {
            return size;
        }
        if (size < MAX_WORDS)
        {
            words[size].string = start;
            words[size].length = (size_t)(text - start);
            size++;
        }
    }
    return 0;
}

struct query
{
    func_callback func[2];
    int optional, unique;
};

static int compare(struct word *word, const char *text)
{
    return (strncmp(text, word->string, word->length) == 0)
        && (strlen(text) == word->length);
}

static int set_func(struct query *query, struct word *word, int id)
{
    size_t size = sizeof map / sizeof map[0];

    for (size_t iter = 0; iter < size; iter++)
    {
        if (compare(word, map[iter].word[id]))
        {
            query->func[id] = map[iter].func;
            return 1;
        }
    }
    return 0;    
}

static int set_prop(struct query *query, struct word *word)
{
    if (compare(word, "optional"))
    {
        if (query->optional == 0)
        {
            query->optional = 1;
            return 1;
        }
    }
    else if (compare(word, "unique"))
    {
        if (query->unique == 0)
        {
            query->unique = 1;
            return 1;
        }
    }
    return 0;    
}

static int set_query(struct query *query, struct word *words, size_t size)
{
    if ((size > 2) && !compare(&words[1], "of"))
    {
        return 0;
    }
    switch (size)
    {
        case 1:
            return set_func(query, &words[0], 0);
        case 3:
            return set_func(query, &words[0], 0)
                && set_func(query, &words[2], 1);    
        case 4:
            return set_func(query, &words[0], 0)
                && set_prop(query, &words[2])
                && set_func(query, &words[3], 1);    
        case 5:
            return set_func(query, &words[0], 0)
                && set_prop(query, &words[2])
                && set_prop(query, &words[3])
                && set_func(query, &words[4], 1);    
        default:
            return 0;
    }
}

static int is_common(const json *node, func_callback func)
{
    while (func(node) != 0)
    {
        node = node->next;
    }
    return node == NULL;
}

static int is_unique(const json *node, func_callback func)
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

    struct word words[MAX_WORDS] = {0};
    size_t size = set_words(words, text);
    struct query query = {0};

    if (!set_query(&query, words, size))
    {
        return 0;
    }

    int rc = query.func[0](node);

    if (rc && query.func[1])
    {
        if (json_is_scalar(node))
        {
            return 0;
        }
        if (node->child == NULL)
        {
            return query.optional;
        }
        return query.unique
            ? is_unique(node->child, query.func[1]) 
            : is_common(node->child, query.func[1]); 
    }
    return rc;
}

