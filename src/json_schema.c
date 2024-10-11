/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h> // remove when no longer needed
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clib_test.h"
#include "clib_string.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_pointer.h"
#include "json_schema.h"

#define SCHEMA_MAX_DEPTH (unsigned)-1

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
#define TEST(X)                                 \
    X(SCHEMA_SCHEMA,        "$schema")          \
    X(SCHEMA_ID,            "$id")              \
    X(SCHEMA_DEFS,          "$defs")            \
    X(SCHEMA_REF,           "$ref")             \
    X(SCHEMA_TITLE,         "title")            \
    X(SCHEMA_DESCRIPTION,   "description")      \
    X(SCHEMA_DEFAULT,       "default")          \
    X(SCHEMA_MIN_LENGTH,    "minLength")        \
    X(SCHEMA_MAX_LENGTH,    "maxLength")

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
    TABLE_SIZE = TESTS_SIZE * 5 
};

static test_t *table[TABLE_SIZE];
static int table_loaded;

static void table_load(void)
{
    for (size_t i = 0; i < TESTS_SIZE; i++)
    {
        unsigned long index = hash(tests[i].name) % TABLE_SIZE;
        test_t *test = table[index];

        if (test != NULL)
        {
            while (test->next != NULL)
            {
                test = test->next;
            }
            test->next = &tests[i];
        }
        else
        {
            table[index] = &tests[i];
        }
    }
    table_loaded = 1;
}

static int test_index(const json_t *rule)
{
    if (rule->key == NULL)
    {
        return SCHEMA_ERROR;
    }

    unsigned long index = hash(rule->key) % TABLE_SIZE;
    test_t *test = table[index];

    while (test != NULL)
    {
        if (!strcmp(test->name, rule->key))
        {
            return (int)(test - tests) + DEFS + 1;
        }
        test = test->next;
    }
    return SCHEMA_WARNING;
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
        int test = test_index(rule->child[i]);

        switch (test)
        {
            case SCHEMA_WARNING:
                if (stoppable && stop_on_warning(schema, rule, node))
                {
                    return SCHEMA_INVALID;
                }
                continue;
            case SCHEMA_INVALID:
                if (stoppable && stop_on_invalid(schema, rule, node))
                {
                    return SCHEMA_INVALID;
                }
                valid = SCHEMA_INVALID;
                break;
            case SCHEMA_ERROR:
                raise_error(schema, rule, node);
                return SCHEMA_ERROR;
            default:
                puts(tests[test - DEFS - 1].name);
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

