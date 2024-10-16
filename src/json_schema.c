/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clib_test.h"
#include "clib_string.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_pointer.h"
#include "json_schema.h"

typedef struct
{
    // Object containing rules
    const json_t *root;
    // User data
    json_validate_callback callback;
    void *data;
} json_schema_t;

static int validate(const json_schema_t *, const json_t *, const json_t *, int);

static int notify(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int event)
{
    int stop = 0;

    if (schema->callback)
    {
        return schema->callback(node, rule, event, schema->data) == 0;
    }
    return stop;
}

static int abort_on_warning(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_WARNING);
}

static int abort_on_failure(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_INVALID);
}

static void raise_error(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    notify(schema, rule, node, JSON_SCHEMA_ERROR);
}

#define hash(key) hash_str((const unsigned char *)(key))
static unsigned long hash_str(const unsigned char *key)
{
    unsigned long hash = 5381;
    unsigned char chr;

    while ((chr = *key++))
    {
        hash = ((hash << 5) + hash) + chr;
    }
    return hash;
}

typedef struct test { const char *name; struct test *next; } test_t;

/* X macro indexing enum and array of tests */
#define TEST(_)                                                 \
    _(SCHEMA_ADDITIONAL_ITEMS,          "additionalItems")      \
    _(SCHEMA_ADDITIONAL_PROPERTIES,     "additionalProperties") \
    _(SCHEMA_ALL_OF,                    "allOf")                \
    _(SCHEMA_ANY_OF,                    "anyOf")                \
    _(SCHEMA_CONST,                     "const")                \
    _(SCHEMA_DEFAULT,                   "default")              \
    _(SCHEMA_DEFS,                      "$defs")                \
    _(SCHEMA_DEPENDENT_REQUIRED,        "dependentRequired")    \
    _(SCHEMA_DEPENDENT_SCHEMAS,         "dependentSchemas")     \
    _(SCHEMA_DEPRECATED,                "deprecated")           \
    _(SCHEMA_DESCRIPTION,               "description")          \
    _(SCHEMA_ELSE,                      "else")                 \
    _(SCHEMA_ENUM,                      "enum")                 \
    _(SCHEMA_EXAMPLES,                  "examples")             \
    _(SCHEMA_EXCLUSIVE_MAXIMUM,         "exclusiveMaximum")     \
    _(SCHEMA_EXCLUSIVE_MINIMUM,         "exclusiveMinimum")     \
    _(SCHEMA_FORMAT,                    "format")               \
    _(SCHEMA_ID,                        "$id")                  \
    _(SCHEMA_IF,                        "if")                   \
    _(SCHEMA_ITEMS,                     "items")                \
    _(SCHEMA_MAXIMUM,                   "maximum")              \
    _(SCHEMA_MAX_ITEMS,                 "maxItems")             \
    _(SCHEMA_MAX_LENGTH,                "maxLength")            \
    _(SCHEMA_MAX_PROPERTIES,            "maxProperties")        \
    _(SCHEMA_MINIMUM,                   "minimum")              \
    _(SCHEMA_MIN_ITEMS,                 "minItems")             \
    _(SCHEMA_MIN_LENGTH,                "minLength")            \
    _(SCHEMA_MIN_PROPERTIES,            "minProperties")        \
    _(SCHEMA_MULTIPLE_OF,               "multipleOf")           \
    _(SCHEMA_NOT,                       "not")                  \
    _(SCHEMA_ONE_OF,                    "oneOf")                \
    _(SCHEMA_PATTERN,                   "pattern")              \
    _(SCHEMA_PATTERN_PROPERTIES,        "patternProperties")    \
    _(SCHEMA_PROPERTIES,                "properties")           \
    _(SCHEMA_READ_ONLY,                 "readOnly")             \
    _(SCHEMA_REF,                       "$ref")                 \
    _(SCHEMA_REQUIRED,                  "required")             \
    _(SCHEMA_SCHEMA,                    "$schema")              \
    _(SCHEMA_THEN,                      "then")                 \
    _(SCHEMA_TITLE,                     "title")                \
    _(SCHEMA_TYPE,                      "type")                 \
    _(SCHEMA_UNIQUE_ITEMS,              "uniqueItems")          \
    _(SCHEMA_WRITE_ONLY,                "writeOnly")

#define TEST_ENUM(a, b) a,
enum
{
    SCHEMA_ERROR = -1, SCHEMA_INVALID, SCHEMA_VALID, SCHEMA_WARNING,
    SCHEMA_FAILURE, SCHEMA_ABORTED,
    DEFS, TEST(TEST_ENUM) NTESTS
};

#define TEST_NAME(a, b) {.name = b},
static test_t tests[] = {TEST(TEST_NAME)};

enum {TABLE_SIZE = NTESTS - DEFS - 1};
static test_t *table[TABLE_SIZE];

__attribute__((constructor)) static void table_load(void)
{
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
        unsigned long index = hash(tests[i].name) % TABLE_SIZE;

        tests[i].next = table[index];
        table[index] = &tests[i];
    }
}

static int table_get_test(const char *key)
{
    unsigned long index = hash(key) % TABLE_SIZE;
    test_t *test = table[index];

    while (test != NULL)
    {
        if (!strcmp(test->name, key))
        {
            return (int)(test - tests) + DEFS + 1;
        }
        test = test->next;
    }
    return SCHEMA_WARNING;
}

static int get_test(const json_t *rule)
{
    if (rule->key == NULL)
    {
        return SCHEMA_ERROR;
    }

    int test = table_get_test(rule->key);

    // Validate very simple rules that doesn't need to be tested
    switch (test)
    {
        case SCHEMA_DEFAULT:
            return SCHEMA_VALID;
        case SCHEMA_DEFS:
            return rule->type == JSON_OBJECT ? SCHEMA_VALID : SCHEMA_ERROR;
        case SCHEMA_EXAMPLES:
            return rule->type == JSON_ARRAY ? SCHEMA_VALID : SCHEMA_ERROR;
        case SCHEMA_DESCRIPTION:
        case SCHEMA_ID:
        case SCHEMA_SCHEMA:
        case SCHEMA_TITLE:
            return rule->type == JSON_STRING ? SCHEMA_VALID : SCHEMA_ERROR;
        case SCHEMA_DEPRECATED:
        case SCHEMA_READ_ONLY:
        case SCHEMA_WRITE_ONLY:
            return (rule->type == JSON_TRUE) || (rule->type == JSON_FALSE)
                ? SCHEMA_VALID
                : SCHEMA_ERROR;
        default:
            return test;
    }
}

static int test_additional_items(const json_schema_t *schema,
    const json_t *parent, const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type == JSON_TRUE)
    {
        return SCHEMA_VALID;
    }
    if ((rule->type != JSON_FALSE) && (rule->type != JSON_OBJECT))
    {
        return SCHEMA_ERROR;
    } 
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }

    const json_t *items = json_find(parent, "items");
    int result = SCHEMA_VALID;

    if (rule->type == JSON_FALSE)
    {
        result = node->size <= items->size;
    }
    else // if (rule->type == JSON_OBJECT)
    {
        for (unsigned i = items->size; i < node->size; i++)
        {
            switch (validate(schema, rule, node->child[i], abortable))
            {
                case SCHEMA_ERROR:
                    return SCHEMA_ABORTED;
                case SCHEMA_INVALID:
                    if (!abortable)
                    {
                        return SCHEMA_FAILURE;
                    }
                    result = SCHEMA_FAILURE;
                    break;
                default:
                    break;
            }
        }
    }
    return result;
}

static int test_additional_properties(const json_schema_t *schema,
    const json_t *parent, const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type == JSON_TRUE)
    {
        return SCHEMA_VALID;
    }
    if ((rule->type != JSON_FALSE) && (rule->type != JSON_OBJECT))
    {
        return SCHEMA_ERROR;
    } 
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    const json_t *properties = json_find(parent, "properties");
    int result = SCHEMA_VALID;

    if (rule->type == JSON_FALSE)
    {
        for (unsigned i = 0; i < node->size; i++)
        {
            if (!json_find(properties, node->child[i]->key))
            { 
                if (!abortable)
                {
                    return SCHEMA_FAILURE;
                }
                if (abort_on_failure(schema, rule, node->child[i]))
                {
                    return SCHEMA_ABORTED;
                }
                result = SCHEMA_FAILURE;
            }
        }
    }
    else // if (rule->type == JSON_OBJECT)
    {
        for (unsigned i = 0; i < node->size; i++)
        {
            if (!json_find(properties, node->child[i]->key))
            { 
                switch (validate(schema, rule, node->child[i], abortable))
                {
                    case SCHEMA_ERROR:
                        return SCHEMA_ABORTED;
                    case SCHEMA_INVALID:
                        if (!abortable)
                        {
                            return SCHEMA_FAILURE;
                        }
                        result = SCHEMA_FAILURE;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return result;
}

static int test_all_of(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_ARRAY) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }
    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }
        switch (validate(schema, rule->child[i], node, 0))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_INVALID:
                return SCHEMA_INVALID;
            default:
                break; 
        }
    }
    return SCHEMA_VALID;
}

static int test_any_of(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_ARRAY) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }
    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }
        switch (validate(schema, rule->child[i], node, 0))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_VALID:
                return SCHEMA_VALID;
            default:
                break; 
        }
    }
    return SCHEMA_INVALID;
}

static int test_branch(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    switch (validate(schema, rule, node, abortable))
    {
        case SCHEMA_ERROR:
            return SCHEMA_ABORTED;
        case SCHEMA_INVALID:
            return SCHEMA_FAILURE;
        default:
            return SCHEMA_VALID;
    }
}

static int test_const(const json_t *rule, const json_t *node)
{
    return json_equal(rule, node);
}

static int test_dependent_schemas(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }
        if (rule->child[i]->size == 0)
        {
            continue;
        }
        if (!json_find(node, rule->child[i]->key))
        {
            continue;
        }
        switch (validate(schema, rule->child[i], node, abortable))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_INVALID:
                if (!abortable)
                {
                    return SCHEMA_FAILURE;
                }
                result = SCHEMA_FAILURE;
                break;
            default:
                break;
        }
    }
    return result;
}

static int test_dependent_required(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if ((rule->type != JSON_OBJECT) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        const json_t *keys = rule->child[i];

        if ((keys->type != JSON_ARRAY) || (keys->size == 0))
        {
            return SCHEMA_ERROR;
        }
        if (!json_find(node, keys->key))
        {
            continue;
        }
        for (unsigned j = 0; j < keys->size; j++)
        {
            if (keys->child[j]->type != JSON_STRING)
            {
                return SCHEMA_ERROR;
            }
            if (!json_find(node, keys->child[j]->string))
            {
                if (abortable)
                {
                    const json_t temp =
                    {
                        .key = rule->key,
                        .child = (json_t *[]){rule->child[i]},
                        .size = 1,
                        .type = JSON_OBJECT
                    };

                    if (abort_on_failure(schema, &temp, node))
                    {
                        return SCHEMA_ABORTED;
                    }
                    result = SCHEMA_FAILURE;
                }
                else
                {
                    return SCHEMA_FAILURE;
                }
            }
        }
    }
    return result;
}

static int test_enum(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_ARRAY) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }
    return json_locate(rule, node) != NULL;
}

static int test_exclusive_maximum(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) && (rule->type != JSON_REAL))
    {
        return SCHEMA_ERROR;
    }
    if ((node->type != JSON_INTEGER) && (node->type != JSON_REAL))
    {
        return SCHEMA_VALID;
    }
    return node->number < rule->number;
}

static int test_exclusive_minimum(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) && (rule->type != JSON_REAL))
    {
        return SCHEMA_ERROR;
    }
    if ((node->type != JSON_INTEGER) && (node->type != JSON_REAL))
    {
        return SCHEMA_VALID;
    }
    return node->number > rule->number;
}

static int test_format(const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_STRING)
    {
        return SCHEMA_ERROR;
    }
    if ((node->type != JSON_STRING) || (node->string[0] == 0))
    {
        return SCHEMA_VALID;
    }
    return test_match(node->string, rule->string);
}

static int test_if(const json_schema_t *schema,
    const json_t *parent, unsigned *child, const json_t *node)
{
    unsigned index = *child;
    const json_t *rule = parent->child[index];

    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (parent->size <= index + 1)
    {
        return SCHEMA_VALID;
    }
    
    const json_t *next = parent->child[index + 1];
    int branch = 0;
 
    if (!strcmp(next->key, "else"))
    {
        branch = 1;
    }
    else if (strcmp(next->key, "then"))
    {
        return SCHEMA_VALID;
    }
    if (next->type != JSON_OBJECT)
    {
        *child += 1;
        return SCHEMA_ERROR;
    }

    int result = validate(schema, rule, node, 0);

    if (result == SCHEMA_ERROR)
    {
        return SCHEMA_ABORTED;
    }
    if (result == branch)
    {
        *child += 1;
    }
    return SCHEMA_VALID;
}

static int test_items(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
        case JSON_ARRAY:
        case JSON_FALSE:
            break;
        case JSON_TRUE:
            return SCHEMA_VALID;
        default:
            return SCHEMA_ERROR;
    }
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }
    if (rule->type == JSON_FALSE)
    {
        return node->size == 0;
    }
    if (rule->size == 0)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    if (rule->type == JSON_OBJECT)
    {
        for (unsigned i = 0; i < node->size; i++)
        {
            switch (validate(schema, rule, node->child[i], abortable))
            {
                case SCHEMA_ERROR:
                    return SCHEMA_ABORTED;
                case SCHEMA_INVALID:
                    if (!abortable)
                    {
                        return SCHEMA_FAILURE;
                    }
                    result = SCHEMA_FAILURE;
                    break;
                default:
                    break;
            }
        }
    }
    else // if (rule->type == JSON_ARRAY)
    {
        for (unsigned i = 0; (i < rule->size) && (i < node->size); i++)
        {
            if (rule->child[i]->type != JSON_OBJECT)
            {
                return SCHEMA_ERROR;
            }
            switch (validate(schema, rule->child[i], node->child[i], abortable))
            {
                case SCHEMA_ERROR:
                    return SCHEMA_ABORTED;
                case SCHEMA_INVALID:
                    if (!abortable)
                    {
                        return SCHEMA_FAILURE;
                    }
                    result = SCHEMA_FAILURE;
                    break;
                default:
                    break;
            }
        }
    }
    return result;
}

static int test_maximum(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) && (rule->type != JSON_REAL))
    {
        return SCHEMA_ERROR;
    }
    if ((node->type != JSON_INTEGER) && (node->type != JSON_REAL))
    {
        return SCHEMA_VALID;
    }
    return node->number <= rule->number;
}

static int test_max_items(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }
    return (size_t)rule->number >= node->size;
}

static int test_max_length(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_STRING)
    {
        return SCHEMA_VALID;
    }
    return (size_t)rule->number >= string_length(node->string);
}

static int test_max_properties(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }
    return (size_t)rule->number >= node->size;
}

static int test_minimum(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) && (rule->type != JSON_REAL))
    {
        return SCHEMA_ERROR;
    }
    if ((node->type != JSON_INTEGER) && (node->type != JSON_REAL))
    {
        return SCHEMA_VALID;
    }
    return node->number >= rule->number;
}

static int test_min_items(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }
    return (size_t)rule->number <= node->size;
}

static int test_min_length(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_STRING)
    {
        return SCHEMA_VALID;
    }
    return (size_t)rule->number <= string_length(node->string);
}

static int test_min_properties(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }
    return (size_t)rule->number <= node->size;
}

static int test_multiple_of(const json_t *rule, const json_t *node)
{
    if (json_number(rule) <= 0)
    {
        return SCHEMA_ERROR;
    }
    if (!json_is_number(node))
    {
        return SCHEMA_VALID;
    }
    return fmod(node->number, rule->number) == 0.0;
}

static int test_not(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (rule->size == 0)
    {
        return SCHEMA_INVALID;
    }
 
    int result = validate(schema, rule, node, 0);

    if (result == SCHEMA_ERROR)
    {
        return SCHEMA_ABORTED;
    }
    return !result;
}

static int test_one_of(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_ARRAY) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }

    int count = 0;

    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }
        switch (validate(schema, rule->child[i], node, 0))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_VALID:
                if (count++ == 1)
                {
                    return SCHEMA_INVALID;
                }
                break;
            default:
                break; 
        }
    }
    return count == 1;
}

static int test_pattern(const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_STRING)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_STRING)
    {
        return SCHEMA_VALID;
    }
    return test_regex(node->string, rule->string);
}

static int test_pattern_properties(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }
        if (rule->child[i]->size == 0)
        {
            continue;
        }
        for (unsigned j = 0; j < node->size; j++)
        {
            if (!test_regex(node->child[j]->key, rule->child[i]->key))
            {
                continue;
            };
            switch (validate(schema, rule->child[i], node->child[j], abortable))
            {
                case SCHEMA_ERROR:
                    return SCHEMA_ABORTED;
                case SCHEMA_INVALID:
                    if (!abortable)
                    {
                        return SCHEMA_FAILURE;
                    }
                    result = SCHEMA_FAILURE;
                    break;
                default:
                    break;
            }
        }
    }
    return result;
}

static int test_properties(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type == JSON_TRUE)
        {
            continue;
        }

        const json_t *property = json_find(node, rule->child[i]->key);

        if (property == NULL)
        {
            continue;
        }
        if (rule->child[i]->type == JSON_FALSE)
        {
            if (!abortable)
            {
                return SCHEMA_FAILURE;
            }
            if (abort_on_failure(schema, rule->child[i], property))
            {
                return SCHEMA_ABORTED;
            }
            result = SCHEMA_FAILURE;
            continue;
        }
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }
        if (rule->child[i]->size == 0)
        {
            continue;
        }
        switch (validate(schema, rule->child[i], property, abortable))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_INVALID:
                if (!abortable)
                {
                    return SCHEMA_FAILURE;
                }
                result = SCHEMA_FAILURE;
                break;
            default:
                break;
        }
    }
    return result;
}

static int test_required(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (rule->type != JSON_ARRAY)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_STRING)
        {
            return SCHEMA_ERROR;
        }
        if (!json_find(node, rule->child[i]->string))
        {
            if (abortable)
            {
                const json_t temp =
                {
                    .key = rule->key,
                    .string = rule->child[i]->string,
                    .type = JSON_STRING
                };

                if (abort_on_failure(schema, &temp, node))
                {
                    return SCHEMA_ABORTED;
                }
                result = SCHEMA_FAILURE;
            }
            else
            {
                return SCHEMA_FAILURE;
            }
        }
    }
    return result;
}

static unsigned add_type(const char *type, unsigned mask)
{
    static const char *types[] =
    {
        "object", "array", "string", "integer", "number", "boolean", "null"
    };
    size_t size = sizeof(types) / sizeof(char *);

    for (size_t item = 0; item < size; item++)
    {
        if (!strcmp(type, types[item]))
        {
            return mask | (1u << (item + 1));
        }
    }
    return 0;
}

static int test_type(const json_t *rule, const json_t *node)
{
    unsigned mask = 0;

    if ((rule->type == JSON_ARRAY) && (rule->size > 0))
    {
        for (unsigned i = 0; i < rule->size; i++)
        {
            if (!(mask = add_type(json_text(rule->child[i]), mask)))
            {
                return SCHEMA_ERROR;
            }
        }
    }
    else if (rule->type == JSON_STRING)
    {
        if (!(mask = add_type(rule->string, mask)))
        {
            return SCHEMA_ERROR;
        }
    }
    else
    {
        return SCHEMA_ERROR;
    }

    unsigned type = node->type;

    /* Reduce JSON_FALSE and JSON_NULL in order to match types */
    return (mask & (1u << (type < JSON_FALSE ? type : type - 1)))
        /* 'integer' validates as true when type is 'number' */
        || ((mask & (1u << JSON_REAL)) && (type == JSON_INTEGER));
}

static int test_unique_items(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_TRUE) && (rule->type != JSON_FALSE))
    {
        return SCHEMA_ERROR;
    }
    if ((rule->type != JSON_TRUE) || (node->type != JSON_ARRAY))
    {
        return SCHEMA_VALID;
    }
    return json_unique_children(node);
}

static int validate(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        int test = get_test(rule->child[i]);

        switch (test)
        {
            // Validate very simple rules that doesn't need to be tested
            case SCHEMA_VALID:
                continue;
            case SCHEMA_WARNING:
                if (abort_on_warning(schema, rule->child[i], node))
                {
                    return SCHEMA_ERROR;
                }
                continue;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ERROR;
            // Validate simple tests
            case SCHEMA_CONST:
                test = test_const(rule->child[i], node);
                break;
            case SCHEMA_ENUM:
                test = test_enum(rule->child[i], node);
                break;
            case SCHEMA_EXCLUSIVE_MAXIMUM:
                test = test_exclusive_maximum(rule->child[i], node);
                break;
            case SCHEMA_EXCLUSIVE_MINIMUM:
                test = test_exclusive_minimum(rule->child[i], node);
                break;
            case SCHEMA_FORMAT:
                test = test_format(rule->child[i], node);
                break;
            case SCHEMA_MAXIMUM:
                test = test_maximum(rule->child[i], node);
                break;
            case SCHEMA_MAX_ITEMS:
                test = test_max_items(rule->child[i], node);
                break;
            case SCHEMA_MAX_LENGTH:
                test = test_max_length(rule->child[i], node);
                break;
            case SCHEMA_MAX_PROPERTIES:
                test = test_max_properties(rule->child[i], node);
                break;
            case SCHEMA_MINIMUM:
                test = test_minimum(rule->child[i], node);
                break;
            case SCHEMA_MIN_ITEMS:
                test = test_min_items(rule->child[i], node);
                break;
            case SCHEMA_MIN_LENGTH:
                test = test_min_length(rule->child[i], node);
                break;
            case SCHEMA_MIN_PROPERTIES:
                test = test_min_properties(rule->child[i], node);
                break;
            case SCHEMA_MULTIPLE_OF:
                test = test_multiple_of(rule->child[i], node);
                break;
            case SCHEMA_PATTERN:
                test = test_pattern(rule->child[i], node);
                break;
            case SCHEMA_TYPE:
                test = test_type(rule->child[i], node);
                break;
            case SCHEMA_UNIQUE_ITEMS:
                test = test_unique_items(rule->child[i], node);
                break;
            // Validate multiple tests
            case SCHEMA_REQUIRED:
                test = test_required(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_DEPENDENT_REQUIRED:
                test = test_dependent_required(schema, rule->child[i], node, abortable);
                break;
            // Validate special case 'not'
            case SCHEMA_NOT:
                test = test_not(schema, rule->child[i], node);
                break;
            // Validate recursive combinators tests
            case SCHEMA_ANY_OF:
                test = test_any_of(schema, rule->child[i], node);
                break;
            case SCHEMA_ONE_OF:
                test = test_one_of(schema, rule->child[i], node);
                break;
            case SCHEMA_ALL_OF:
                test = test_all_of(schema, rule->child[i], node);
                break;
            // Validate recursive logical tests
            case SCHEMA_IF:
                test = test_if(schema, rule, &i, node);
                break;
            case SCHEMA_THEN:
            case SCHEMA_ELSE:
                test = test_branch(schema, rule->child[i], node, abortable);
                break;
            // Validate recursive object related tests
            case SCHEMA_PROPERTIES:
                test = test_properties(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_ADDITIONAL_PROPERTIES:
                test = test_additional_properties(schema, rule, rule->child[i], node, abortable);
                break;
            case SCHEMA_PATTERN_PROPERTIES:
                test = test_pattern_properties(schema, rule->child[i], node, abortable);
                break;
            // Validate recursive array related tests
            case SCHEMA_ITEMS:
                test = test_items(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_ADDITIONAL_ITEMS:
                test = test_additional_items(schema, rule, rule->child[i], node, abortable);
                break;
            // Validate recursive dependent tests
            case SCHEMA_DEPENDENT_SCHEMAS:
                test = test_dependent_schemas(schema, rule->child[i], node, abortable);
                break;
            // Rule not handled (shouldn't get here)
            default:
                assert(0 && "Unhandled test case");
                //test = SCHEMA_ERROR;
        }
        // Validate result
        switch (test)
        {
            case SCHEMA_INVALID:
                if (abortable && abort_on_failure(schema, rule->child[i], node))
                {
                    return SCHEMA_ERROR;
                }
                result = SCHEMA_INVALID;
                break;
            case SCHEMA_FAILURE:
                result = SCHEMA_INVALID;
                break;
            case SCHEMA_ABORTED:
                return SCHEMA_ERROR;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ERROR;
            default:
                break;
        }
    }
    return result;
}

int json_validate(const json_t *node, const json_t *rule,
    json_validate_callback callback, void *data)
{
    const json_schema_t schema =
    {
        .root = rule,
        .callback = callback,
        .data = data
    };

    if ((rule == NULL) || (rule->type != JSON_OBJECT) || (node == NULL))
    {
        return 0;
    }
    return validate(&schema, rule, node, 1) > 0;
}

