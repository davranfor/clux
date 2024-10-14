/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>

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

static int stop_on_warning(const json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_WARNING);
}

static int stop_on_invalid(const json_schema_t *schema,
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
    _(SCHEMA_TITLE,                     "title")                \
    _(SCHEMA_TYPE,                      "type")                 \
    _(SCHEMA_UNIQUE_ITEMS,              "uniqueItems")          \
    _(SCHEMA_WRITE_ONLY,                "writeOnly")

#define TEST_ENUM(a, b) a,
enum
{
    SCHEMA_ERROR = -1, SCHEMA_INVALID, SCHEMA_VALID, SCHEMA_WARNING,
    SCHEMA_INVALID_CONTINUE, SCHEMA_INVALID_STOP,
    DEFS = SCHEMA_WARNING, TEST(TEST_ENUM) NTESTS
};

#define TEST_NAME(a, b) {.name = b},
static test_t tests[] = {TEST(TEST_NAME)};

enum
{
    TESTS_SIZE = NTESTS - DEFS - 1,
    TABLE_SIZE = TESTS_SIZE * 3 
};

static test_t *table[TABLE_SIZE];

__attribute__((constructor)) static void table_load(void)
{
    for (size_t i = 0; i < TESTS_SIZE; i++)
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
            return rule->type == JSON_OBJECT
                ? SCHEMA_VALID
                : SCHEMA_ERROR;
        case SCHEMA_EXAMPLES:
            return rule->type == JSON_ARRAY
                ? SCHEMA_VALID
                : SCHEMA_ERROR;
        case SCHEMA_DESCRIPTION:
        case SCHEMA_ID:
        case SCHEMA_SCHEMA:
        case SCHEMA_TITLE:
            return rule->type == JSON_STRING
                ? SCHEMA_VALID
                : SCHEMA_ERROR;
        case SCHEMA_DEPRECATED:
        case SCHEMA_READ_ONLY:
        case SCHEMA_WRITE_ONLY:
            return rule->type == JSON_TRUE
                ? SCHEMA_VALID
                : rule->type == JSON_FALSE
                    ? SCHEMA_VALID
                    : SCHEMA_ERROR;
        default:
            return test;
    }
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
                return SCHEMA_INVALID_STOP;
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
                return SCHEMA_INVALID_STOP;
            case SCHEMA_VALID:
                return SCHEMA_VALID;
            default:
                break; 
        }
    }
    return SCHEMA_INVALID;
}

static int test_const(const json_t *rule, const json_t *node)
{
    return json_equal(rule, node);
}

static int test_dependent_required(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int stoppable)
{
    if ((rule->type != JSON_OBJECT) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int valid = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        const json_t *elem = rule->child[i];

        if ((elem->type != JSON_ARRAY) || (elem->size == 0))
        {
            return SCHEMA_ERROR;
        }
        if (!json_find(node, elem->key))
        {
            continue;
        }
        for (unsigned j = 0; j < elem->size; j++)
        {
            if (elem->child[j]->type != JSON_STRING)
            {
                return SCHEMA_ERROR;
            }
            if (!json_find(node, elem->child[j]->string))
            {
                if (stoppable)
                {
                    const json_t temp =
                    {
                        .key = rule->key,
                        .child = (json_t *[]){rule->child[i]},
                        .size = 1,
                        .type = JSON_OBJECT
                    };

                    if (stop_on_invalid(schema, &temp, node))
                    {
                        return SCHEMA_INVALID_STOP;
                    }
                }
                valid = SCHEMA_INVALID_CONTINUE;
                break;
            }
        }
    }
    return valid;
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
        return SCHEMA_INVALID_STOP;
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
                return SCHEMA_INVALID_STOP;
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

static int test_properties(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int stoppable)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int valid = SCHEMA_VALID;

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

        const json_t *elem = json_find(node, rule->child[i]->key);

        if (elem == NULL)
        {
            continue;
        }
        switch (validate(schema, rule->child[i], elem, stoppable))
        {
            case SCHEMA_ERROR:
                return SCHEMA_INVALID_STOP;
            case SCHEMA_INVALID:
                valid = SCHEMA_INVALID_CONTINUE;
                break;
            default:
                break;
        }
    }
    return valid;
}

static int test_required(const json_schema_t *schema,
    const json_t *rule, const json_t *node, int stoppable)
{
    if (rule->type != JSON_ARRAY)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int valid = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_STRING)
        {
            return SCHEMA_ERROR;
        }
        if (!json_find(node, rule->child[i]->string))
        {
            if (stoppable)
            {
                const json_t temp =
                {
                    .key = rule->key,
                    .string = rule->child[i]->string,
                    .type = JSON_STRING
                };

                if (stop_on_invalid(schema, &temp, node))
                {
                    return SCHEMA_INVALID_STOP;
                }
            }
            valid = SCHEMA_INVALID_CONTINUE;
        }
    }
    return valid;
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

    if (rule->type == JSON_STRING)
    {
        if (!(mask = add_type(rule->string, mask)))
        {
            return SCHEMA_ERROR;
        }
    }
    else if ((rule->type == JSON_ARRAY) && (rule->size > 0))
    {
        for (unsigned i = 0; i < rule->size; i++)
        {
            if (!(mask = add_type(json_text(rule->child[i]), mask)))
            {
                return SCHEMA_ERROR;
            }
        }
    }
    else
    {
        return SCHEMA_ERROR;
    }

    unsigned type = node->type;

    /* Reduce JSON_FALSE and JSON_NULL in order to match types */
    return (mask & (1u << (type < JSON_FALSE ? type : type - 1)))
        /* 'integer' validates as true if type is 'number' */
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
    const json_t *rule, const json_t *node, int stoppable)
{
    int valid = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        int test = get_test(rule->child[i]);

        switch (test)
        {
            // Validate very simple rules that doesn't need to be tested
            case SCHEMA_VALID:
                continue;
            case SCHEMA_WARNING:
                if (stop_on_warning(schema, rule->child[i], node))
                {
                    return SCHEMA_ERROR;
                }
                continue;
            case SCHEMA_INVALID:
                if (stoppable && stop_on_invalid(schema, rule->child[i], node))
                {
                    return SCHEMA_ERROR;
                }
                valid = SCHEMA_INVALID;
                break;
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
            case SCHEMA_DEPENDENT_REQUIRED:
                test = test_dependent_required(schema, rule->child[i], node, stoppable);
                break;
            case SCHEMA_REQUIRED:
                test = test_required(schema, rule->child[i], node, stoppable);
                break;
            // Validate recursive tests forcing stoppable to 'false' (not)
            case SCHEMA_NOT:
                test = test_not(schema, rule->child[i], node);
                break;
            // Validate recursive tests forcing stoppable to 'false' (combinators)
            case SCHEMA_ALL_OF:
                test = test_all_of(schema, rule->child[i], node);
                break;
            case SCHEMA_ANY_OF:
                test = test_any_of(schema, rule->child[i], node);
                break;
            case SCHEMA_ONE_OF:
                test = test_one_of(schema, rule->child[i], node);
                break;
            // Validate recursive tests
            case SCHEMA_PROPERTIES:
                test = test_properties(schema, rule->child[i], node, stoppable);
                break;
            default:
                assert("Unhandled case");
                //test = SCHEMA_ERROR;
        }
        // Validate result
        switch (test)
        {
            case SCHEMA_VALID:
                continue;
            case SCHEMA_INVALID:
                if (stoppable && stop_on_invalid(schema, rule->child[i], node))
                {
                    return SCHEMA_ERROR;
                }
                valid = SCHEMA_INVALID;
                break;
            case SCHEMA_INVALID_CONTINUE:
                valid = SCHEMA_INVALID;
                break;
            case SCHEMA_INVALID_STOP:
                return SCHEMA_ERROR;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ERROR;
            default:
                break;
        }
    }
    return valid;
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

