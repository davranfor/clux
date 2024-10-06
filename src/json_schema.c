/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "clib_test.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_pointer.h"
#include "json_schema.h"

#define SCHEMA_MAX_DEPTH (unsigned)-1

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

typedef struct
{
    const json_t *root;
    // Recursion
    const json_t *skip;
    unsigned depth;
    // User data
    json_schema_callback callback;
    void *data;
} json_schema_t;

static int test_is_object(const json_t *rule)
{
    return rule->type == JSON_OBJECT
        ? SCHEMA_VALID
        : SCHEMA_ERROR;
}

static int test_is_string(const json_t *rule)
{
    return rule->type == JSON_STRING
        ? SCHEMA_VALID
        : SCHEMA_ERROR;
}

static int test_ref(const json_t *rule)
{
    return rule->type == JSON_STRING
        ? SCHEMA_REF
        : SCHEMA_ERROR;
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

typedef int (*func0)(void);
typedef int (*func1)(const json_t *);
typedef int (*func2)(const json_t *, const json_t *);
typedef int (*func3)(const json_t *, const json_t *, const json_t *);

typedef struct schema_func
{
    size_t params;
    union {func0 fn0; func1 fn1; func2 fn2; func3 fn3;};
    const char *name;
    struct schema_func *next;
} func_t;

static func_t funcs[] =
{
    {.params = 1,   .fn1 = test_is_string,   .name = "$schema"},
    {.params = 1,   .fn1 = test_is_string,   .name = "$id"},
    {.params = 1,   .fn1 = test_is_object,   .name = "$defs"},
    {.params = 1,   .fn1 = test_ref,         .name = "$ref"},
    {.params = 1,   .fn1 = test_is_string,   .name = "title"},
    {.params = 1,   .fn1 = test_is_string,   .name = "description"},
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
}

/*
        equal(name, "$schema") ? test_is_string :
        equal(name, "$id") ? test_is_string :
        equal(name, "$defs") ? test_is_object :
        equal(name, "$ref") ? test_ref :
        equal(name, "title") ? test_is_string :
        equal(name, "description") ? test_is_string :
        equal(name, "not") ? test_not :
        equal(name, "allOf") ? test_all_of :
        equal(name, "anyOf") ? test_any_of :
        equal(name, "oneOf") ? test_one_of :
        equal(name, "if") ? test_if :
        equal(name, "then") ? test_then :
        equal(name, "else") ? test_else :
        equal(name, "type") ? test_type :
        equal(name, "const") ? test_const :
        equal(name, "enum") ? test_enum :
        equal(name, "required") ? test_required :
        equal(name, "dependentRequired") ? test_dependent_required :
        equal(name, "dependentSchemas") ? test_dependent_schemas :
        equal(name, "properties") ? test_properties :
        equal(name, "patternProperties") ? test_pattern_properties :
        equal(name, "additionalProperties") ? test_additional_properties :
        equal(name, "minProperties") ? test_min_properties :
        equal(name, "maxProperties") ? test_max_properties :
        equal(name, "items") ? test_items :
        equal(name, "additionalItems") ? test_additional_items :
        equal(name, "minItems") ? test_min_items :
        equal(name, "maxItems") ? test_max_items :
        equal(name, "uniqueItems") ? test_unique_items :
        equal(name, "minLength") ? test_min_length :
        equal(name, "maxLength") ? test_max_length :
        equal(name, "format") ? test_format :
        equal(name, "pattern") ? test_pattern :
        equal(name, "minimum") ? test_minimum :
        equal(name, "maximum") ? test_maximum :
        equal(name, "exclusiveMinimum") ? test_is_boolean :
        equal(name, "exclusiveMaximum") ? test_is_boolean :
        equal(name, "multipleOf") ? test_multiple_of :
        equal(name, "readOnly") ? test_is_boolean :
        equal(name, "writeOnly") ? test_is_boolean :
        equal(name, "deprecated") ? test_is_boolean :
        equal(name, "examples") ? test_is_array :
        equal(name, "default") ? test_valid : NULL;
*/

static int validate(json_schema_t *schema,
    const json_t *node, const json_t *rule)
{
    (void)node;
    (void)rule;

    int valid = 1;

    if (schema->depth++ >= SCHEMA_MAX_DEPTH)
    {

    }
    schema->depth--;
    return valid;
}

int json_validate(const json_t *node, const json_t *rule,
    json_schema_callback callback, void *data)
{
    json_schema_t schema =
    {
        .root = rule,
        .callback = callback,
        .data = data
    };

    if ((node == NULL) || (rule == NULL))
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
    return validate(&schema, node, rule);
}

