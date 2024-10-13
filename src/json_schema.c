/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h> // remove when no longer needed
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

#define SCHEMA_MAX_DEPTH (unsigned short)-1

typedef struct
{
    // Object containing rules
    const json_t *root;
    // Detect infinite loops
    unsigned depth;
    // User data
    json_validate_callback callback;
    void *data;
} json_schema_t;

static int notify(json_schema_t *schema,
    const json_t *rule, const json_t *node, int event)
{
    int stop = 0;

    if (schema->callback)
    {
        return schema->callback(node, rule, event, schema->data) == 0;
    }
    return stop;
}

static int stop_on_warning(json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_WARNING);
}

static int stop_on_invalid(json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_INVALID);
}

static void raise_error(json_schema_t *schema,
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
    DEFS = SCHEMA_WARNING, TEST(TEST_ENUM) NTESTS
};

#define TEST_NAME(a, b) {.name = b},
static test_t tests[] = {TEST(TEST_NAME)};

enum
{
    TESTS_SIZE = NTESTS - DEFS - 1,
    TABLE_SIZE = TESTS_SIZE 
};

static test_t *table[TABLE_SIZE];
static int table_loaded;

static void table_load(void)
{
    for (size_t i = 0; i < TESTS_SIZE; i++)
    {
        unsigned long index = hash(tests[i].name) % TABLE_SIZE;

        tests[i].next = table[index];
        table[index] = &tests[i];
    }
    table_loaded = 1;
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
        case SCHEMA_EXCLUSIVE_MAXIMUM:
        case SCHEMA_EXCLUSIVE_MINIMUM:
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

static unsigned add_type(const char *type, unsigned value)
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
            return value | (1u << (item + 1));
        }
    }
    return 0;
}

static int validate_type(const json_t *rule, const json_t *node)
{
    unsigned mask = 0;

    if (rule->type == JSON_STRING)
    {
        if (!(mask = add_type(rule->string, mask)))
        {
            return SCHEMA_ERROR;
        }
    }
    else if (rule->type == JSON_ARRAY)
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
    if (node != NULL)
    {
        unsigned type = node->type;

        /* Reduce JSON_FALSE and JSON_NULL in order to match types */
        return (mask & (1u << (type < JSON_FALSE ? type : type - 1)))
            /* 'integer' validates as true if type is 'number' */
            || ((mask & (1u << JSON_REAL)) && (type == JSON_INTEGER));
    }
    return SCHEMA_VALID;
}

static int validate_unsigned(int test, const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if (node == NULL)
    {
        return SCHEMA_VALID;
    }
    switch (test)
    {
        case SCHEMA_MAX_ITEMS:
            return (node->type != JSON_ARRAY)
                || ((size_t)node->size <= (size_t)rule->number);
        case SCHEMA_MAX_LENGTH:
            return (node->type != JSON_STRING)
                || (string_length(node->string) <= (size_t)rule->number);
        case SCHEMA_MAX_PROPERTIES:
            return (node->type != JSON_OBJECT)
                || ((size_t)node->size <= (size_t)rule->number);
        case SCHEMA_MIN_ITEMS:
            return (node->type != JSON_ARRAY)
                || ((size_t)node->size >= (size_t)rule->number);
        case SCHEMA_MIN_LENGTH:
            return (node->type != JSON_STRING)
                || (string_length(node->string) >= (size_t)rule->number);
        case SCHEMA_MIN_PROPERTIES:
            return (node->type != JSON_OBJECT)
                || ((size_t)node->size >= (size_t)rule->number);
        default:
            assert("Unhandled case");
            return SCHEMA_ERROR;
    }
}

static int validate_required(json_schema_t *schema,
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
                    .key = "required",
                    .string = rule->child[i]->string,
                    .type = JSON_STRING
                };

                if (stop_on_invalid(schema, &temp, node))
                {
                    return SCHEMA_INVALID;
                }
            }
            valid = SCHEMA_INVALID;
        }
    }
    return valid;
}


static int validate(json_schema_t *schema,
    const json_t *rule, const json_t *node, int stoppable)
{
    int valid = SCHEMA_VALID;

    if (schema->depth++ >= SCHEMA_MAX_DEPTH)
    {
        raise_error(schema, rule, node);
        return SCHEMA_ERROR;
    }
    for (unsigned i = 0; i < rule->size; i++)
    {
        int test = get_test(rule->child[i]);

        /* DEBUG (remove this) */
        if (test > DEFS)
        {
            for (unsigned depth = 0; depth - 1 < schema->depth; depth++)
            {
                printf("  ");
            } 
            puts(tests[test - DEFS - 1].name);
        }
        switch (test)
        {
            // Validate very simple rules that doesn't need to be tested
            case SCHEMA_VALID:
                continue;
            case SCHEMA_WARNING:
                if (stoppable && stop_on_warning(schema, rule->child[i], node))
                {
                    return SCHEMA_INVALID;
                }
                continue;
            case SCHEMA_INVALID:
                if (stoppable && stop_on_invalid(schema, rule->child[i], node))
                {
                    return SCHEMA_INVALID;
                }
                valid = SCHEMA_INVALID;
                break;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ERROR;
            // Validate simple tests that doesn't need a parent
            case SCHEMA_TYPE:
                test = validate_type(rule->child[i], node);
                break;
            case SCHEMA_MAX_ITEMS:
            case SCHEMA_MAX_LENGTH:
            case SCHEMA_MAX_PROPERTIES:
            case SCHEMA_MIN_ITEMS:
            case SCHEMA_MIN_LENGTH:
            case SCHEMA_MIN_PROPERTIES:
                test = validate_unsigned(test, rule->child[i], node);
                break;
            // Validate multiple tests that doesn't need a parent
            case SCHEMA_REQUIRED:
                test = validate_required(schema, rule->child[i], node, stoppable);
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
                    return SCHEMA_INVALID;
                }
                valid = SCHEMA_INVALID;
                break;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ERROR;
            default:
                break;
        }
    }
    schema->depth--;
    return valid;
}

int json_validate(const json_t *node, const json_t *rule,
    json_validate_callback callback, void *data)
{
    json_schema_t schema =
    {
        .root = rule,
        .callback = callback,
        .data = data
    };

    if ((rule == NULL) || (node == NULL))
    {
        return 0;
    }
    if (rule->type != JSON_OBJECT)
    {
        return 0;
    }
    if (!table_loaded)
    {
        table_load();
    }
    return validate(&schema, rule, node, 1) > 0;
}

