/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

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

enum
{
    SCHEMA_INVALID, SCHEMA_VALID, SCHEMA_ERROR,
    SCHEMA_DEPENDENT_SCHEMAS,
    SCHEMA_PROPERTIES, SCHEMA_PATTERN_PROPERTIES, SCHEMA_ADDITIONAL_PROPERTIES,
    SCHEMA_ITEMS, SCHEMA_ADDITIONAL_ITEMS, SCHEMA_TUPLES,
    SCHEMA_REF,
    SCHEMA_NOT,
    SCHEMA_ALL_OF, SCHEMA_ANY_OF, SCHEMA_ONE_OF,
    SCHEMA_IF, SCHEMA_THEN, SCHEMA_ELSE
};

static int notify(json_schema_t *schema,
    const json_t *rule, const json_t *node, int event)
{
    int stop = 0;

    if (schema->callback)
    {
        return schema->callback(rule, node, event, schema->data) == 0;
    }
    return stop;
}

static int stop_on_warning(json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_WARNING);
}

static int stop_on_failure(json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_INVALID);
}

static void raise_error(json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    notify(schema, rule, node, JSON_SCHEMA_ERROR);
}

static int test_valid(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema, (void)rule, (void)node;
    return SCHEMA_VALID;
}

static int test_error(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema, (void)rule, (void)node;
    return SCHEMA_ERROR;
}

static int test_is_object(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema, (void)node;
    return rule->type == JSON_OBJECT
        ? SCHEMA_VALID
        : SCHEMA_ERROR;
}

static int test_is_string(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema, (void)node;
    return rule->type == JSON_STRING
        ? SCHEMA_VALID
        : SCHEMA_ERROR;
}

static int test_ref(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema, (void)node;
    return rule->type == JSON_STRING
        ? SCHEMA_REF
        : SCHEMA_ERROR;
}

static int test_min_length(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema;
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return string_length(node->string) >= json_size_t(rule);
    }
    return SCHEMA_VALID;
}

static int test_max_length(const json_t *subschema,
    const json_t *rule, const json_t *node)
{
    (void)subschema;
    if ((rule->type != JSON_INTEGER) || (rule->number < 0))
    {
        return SCHEMA_ERROR;
    }
    if ((node != NULL) && (node->type == JSON_STRING))
    {
        return string_length(node->string) <= json_size_t(rule);
    }
    return SCHEMA_VALID;
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

typedef int (*test_t)(const json_t *, const json_t *, const json_t *);

typedef struct func
{
    const char *name;
    const test_t test;
    struct func *next;
} func_t;

static func_t funcs[] =
{
    {.name = "$schema", .test = test_is_string},
    {.name = "$id", .test = test_is_string},
    {.name = "$defs", .test = test_is_object},
    {.name = "$ref", .test = test_ref},
    {.name = "title", .test = test_is_string},
    {.name = "description", .test = test_is_string},
    {.name = "default", .test = test_valid},
    {.name = "minLength", .test = test_min_length},
    {.name = "maxLength", .test = test_max_length},
};

enum
{
    FUNCS_SIZE = sizeof(funcs) / sizeof(funcs[0]),
    TABLE_SIZE = FUNCS_SIZE * 5 
};

static func_t *table[TABLE_SIZE];
static int table_loaded;

static void table_load(void)
{
    for (size_t i = 0; i < FUNCS_SIZE; i++)
    {
        unsigned long index = hash(funcs[i].name) % TABLE_SIZE;
        func_t *func = table[index];

        if (func != NULL)
        {
            while (func->next != NULL)
            {
                func = func->next;
            }
            func->next = &funcs[i];
        }
        else
        {
            table[index] = &funcs[i];
        }
    }
    table_loaded = 1;
}

static test_t get_test(const json_t *rule)
{
    if (rule->key == NULL)
    {
        return test_error;
    }

    unsigned long index = hash(rule->key) % TABLE_SIZE;
    func_t *func = table[index];

    while (func != NULL)
    {
        if (!strcmp(func->name, rule->key))
        {
            return func->test;
        }
        func = func->next;
    }
    return NULL;
}

static int validate(json_schema_t *schema,
    const json_t *rule, const json_t *node)
{
    int valid = 1;

    if (schema->depth++ >= SCHEMA_MAX_DEPTH)
    {
        raise_error(schema, rule, node);
        return 0;
    }
    for (unsigned i = 0; i < rule->size; i++)
    {
        test_t test = get_test(rule->child[i]);

        if (test == NULL)
        {
            if (stop_on_warning(schema, rule, node))
            {
                return 0;
            }
            continue;
        }
        switch (test(rule, rule->child[i], node))
        {
            case SCHEMA_INVALID:
                if (stop_on_failure(schema, rule, node))
                {
                    return 0;
                }
                valid = 0;
                break;
            case SCHEMA_ERROR:
                raise_error(schema, rule, node);
                return 0;
            default:
                break;
        }
    }
    schema->depth--;
    return valid;
}

int json_validate(const json_t *rule, const json_t *node,
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
    return validate(&schema, rule, node);
}

