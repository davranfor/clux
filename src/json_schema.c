/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "clib_string.h"
#include "clib_match.h"
#include "clib_regex.h"
#include "json_private.h"
#include "json_reader.h"
#include "json_buffer.h"
#include "json_schema.h"

#define MAX_PATHS 32
#define MAX_REFS 128

enum {NOT_ABORTABLE, ABORTABLE};

struct tracker
{
    unsigned path[MAX_PATHS];
    unsigned paths;
    unsigned refs;
};

typedef struct
{
    // Pointers to roots
    const json_t *rule, *node;
    // External schema maps
    const map_t *map;
    // User defined event handler
    json_validate_callback callback;
    void *data;
    // Paths and reference counters
    struct tracker * const active;
} schema_t;

static enum json_warning_mode warning_mode = JSON_WARNINGS_ON;

void json_set_warning_mode(enum json_warning_mode mode)
{
    warning_mode = mode;
}

enum json_warning_mode json_get_warning_mode(void)
{
    return warning_mode;
}

/* Notifies an event to the user-defined callback */
static int notify_event(const schema_t *schema, const json_t *rule, const json_t *node,
    enum json_event_type type)
{
    const json_pointer_t pointer =
    {
        .root = schema->node,
        .path = schema->active->path,
        .size = schema->active->paths > MAX_PATHS ? MAX_PATHS : schema->active->paths
    };
    const json_event_t event =
    {
        .pointer = &pointer, .node = node, .rule = rule, .type = type
    };

    return schema->callback(&event, schema->data);
}

static int notify(const schema_t *schema, const json_t *rule, const json_t *node,
    enum json_event_type event)
{
    return schema->callback ? notify_event(schema, rule, node, event)
                            : event == JSON_FAILURE;
}

static int abort_on_notify(const schema_t *schema, const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_NOTIFY);
}

static int abort_on_warning(const schema_t *schema, const json_t *rule, const json_t *node)
{
    switch (warning_mode)
    {
        case JSON_WARNINGS_OFF:
            return 0;
        case JSON_WARNINGS_ON:
            return notify(schema, rule, node, JSON_WARNING);
        case JSON_WARNING_AS_ERROR:
            notify(schema, rule, node, JSON_ERROR);
            return 1;
        default:
            return 1;
    }
}

static int abort_on_failure(const schema_t *schema, const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_FAILURE);
}

static void raise_error(const schema_t *schema, const json_t *rule, const json_t *node)
{
    notify(schema, rule, node, JSON_ERROR);
}

/**
 * Writes an event to a provided buffer from an user-defined callback
 * Limits the buffer to 'encode_max' bytes when encoding (0 = no limit)
 */
char *json_write_event(buffer_t *buffer, const json_event_t *event, size_t encode_max)
{
    if ((buffer == NULL) || (event == NULL))
    {
        return NULL;
    }
    buffer_write(buffer, "\nPath: ");
    json_write_pointer_max(buffer, event->pointer, encode_max);
    buffer_write(buffer, "\nNode: ");
    json_buffer_encode_max(buffer, event->node, 0, encode_max);
    buffer_write(buffer, "\nRule: ");
    json_buffer_encode_max(buffer, event->rule, 0, encode_max);
    return buffer_put(buffer, '\n');
}

static int validate(const schema_t *, const json_t *, const json_t *, int);

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

typedef struct test { const char *key; struct test *next; } test_t;

/**
 * ------------------------------------------------------------------
 * X macro indexing enum and array of tests
 * Schema (Draft 07) keywords
 * https://json-schema.org/draft-07/schema
 * ------------------------------------------------------------------
 * NOTE: x-mask is an extension intended to avoid expensive regexp's
 *       x-notify is an extension delegating the rule to the callback
 */
#define TEST(_)                                                     \
    _(SCHEMA_ADDITIONAL_ITEMS,          "additionalItems")          \
    _(SCHEMA_ADDITIONAL_PROPERTIES,     "additionalProperties")     \
    _(SCHEMA_ALL_OF,                    "allOf")                    \
    _(SCHEMA_ANY_OF,                    "anyOf")                    \
    _(SCHEMA_COMMENT,                   "$comment")                 \
    _(SCHEMA_CONST,                     "const")                    \
    _(SCHEMA_CONTAINS,                  "contains")                 \
    _(SCHEMA_CONTENT_ENCODING,          "contentEncoding")          \
    _(SCHEMA_CONTENT_MEDIA_TYPE,        "contentMediaType")         \
    _(SCHEMA_DEFAULT,                   "default")                  \
    _(SCHEMA_DEFINITIONS,               "definitions")              \
    _(SCHEMA_DEPENDENCIES,              "dependencies")             \
    _(SCHEMA_DEPRECATED,                "deprecated")               \
    _(SCHEMA_DESCRIPTION,               "description")              \
    _(SCHEMA_ELSE,                      "else")                     \
    _(SCHEMA_ENUM,                      "enum")                     \
    _(SCHEMA_EXAMPLES,                  "examples")                 \
    _(SCHEMA_EXCLUSIVE_MAXIMUM,         "exclusiveMaximum")         \
    _(SCHEMA_EXCLUSIVE_MINIMUM,         "exclusiveMinimum")         \
    _(SCHEMA_FORMAT,                    "format")                   \
    _(SCHEMA_ID,                        "$id")                      \
    _(SCHEMA_IF,                        "if")                       \
    _(SCHEMA_ITEMS,                     "items")                    \
    _(SCHEMA_MAX_ITEMS,                 "maxItems")                 \
    _(SCHEMA_MAX_LENGTH,                "maxLength")                \
    _(SCHEMA_MAX_PROPERTIES,            "maxProperties")            \
    _(SCHEMA_MAXIMUM,                   "maximum")                  \
    _(SCHEMA_MIN_ITEMS,                 "minItems")                 \
    _(SCHEMA_MIN_LENGTH,                "minLength")                \
    _(SCHEMA_MIN_PROPERTIES,            "minProperties")            \
    _(SCHEMA_MINIMUM,                   "minimum")                  \
    _(SCHEMA_MULTIPLE_OF,               "multipleOf")               \
    _(SCHEMA_NOT,                       "not")                      \
    _(SCHEMA_ONE_OF,                    "oneOf")                    \
    _(SCHEMA_PATTERN,                   "pattern")                  \
    _(SCHEMA_PATTERN_PROPERTIES,        "patternProperties")        \
    _(SCHEMA_PROPERTIES,                "properties")               \
    _(SCHEMA_PROPERTY_NAMES,            "propertyNames")            \
    _(SCHEMA_READ_ONLY,                 "readOnly")                 \
    _(SCHEMA_REF,                       "$ref")                     \
    _(SCHEMA_REQUIRED,                  "required")                 \
    _(SCHEMA_SCHEMA,                    "$schema")                  \
    _(SCHEMA_THEN,                      "then")                     \
    _(SCHEMA_TITLE,                     "title")                    \
    _(SCHEMA_TYPE,                      "type")                     \
    _(SCHEMA_UNIQUE_ITEMS,              "uniqueItems")              \
    _(SCHEMA_WRITE_ONLY,                "writeOnly")                \
    _(SCHEMA_X_MASK,                    "x-mask")                   \
    _(SCHEMA_X_NOTIFY,                  "x-notify")

#define TEST_ENUM(a, b) a,
enum
{
    SCHEMA_INVALID, SCHEMA_VALID, SCHEMA_WARNING, SCHEMA_ERROR,
    TESTS, TEST(TEST_ENUM) NTESTS, SCHEMA_ABORT = ~SCHEMA_ERROR
};

#define TEST_KEY(a, b) {.key = b},
static test_t tests[] = {TEST(TEST_KEY)};

enum {TABLE_SIZE = NTESTS - TESTS - 1};
static test_t *table[TABLE_SIZE];

__attribute__((constructor))
static void table_load(void)
{
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
        unsigned long index = hash(tests[i].key) % TABLE_SIZE;

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
        if (!strcmp(test->key, key))
        {
            return TESTS + (int)(test - tests) + 1;
        }
        test = test->next;
    }
    return SCHEMA_WARNING;
}

static int get_test(const json_t *rule)
{
    assert(rule->key != NULL);

    int test = table_get_test(rule->key);

    switch (test)
    {
        // Rules that doesn't need to be tested
        case SCHEMA_DEFAULT:
            return SCHEMA_VALID;
        case SCHEMA_DEFINITIONS:
            return rule->type == JSON_OBJECT ? SCHEMA_VALID : SCHEMA_ERROR;
        case SCHEMA_EXAMPLES:
            return rule->type == JSON_ARRAY ? SCHEMA_VALID : SCHEMA_ERROR;
        case SCHEMA_COMMENT:
        case SCHEMA_CONTENT_ENCODING:
        case SCHEMA_CONTENT_MEDIA_TYPE:
        case SCHEMA_DESCRIPTION:
        case SCHEMA_ID:
        case SCHEMA_SCHEMA:
        case SCHEMA_TITLE:
            return rule->type == JSON_STRING ? SCHEMA_VALID : SCHEMA_ERROR;
        case SCHEMA_DEPRECATED:
        case SCHEMA_READ_ONLY:
        case SCHEMA_WRITE_ONLY:
            return rule->type & JSON_BOOLEAN ? SCHEMA_VALID : SCHEMA_ERROR;
        // Rules that need to be tested
        default:
            return test;
    }
}

static int test_abort(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
{
    if (abortable && abort_on_failure(schema, rule, node))
    {
        return SCHEMA_ABORT;
    }
    return !abortable ? ~SCHEMA_INVALID : SCHEMA_INVALID;
}

static int test_false(const schema_t *schema, const json_t *rule, const json_t *parent,
    unsigned child, int abortable)
{
    if (schema->active->paths++ < MAX_PATHS)
    {
        schema->active->path[schema->active->paths - 1] = child;
    }

    int result = test_abort(schema, rule, parent->child[child], abortable);

    schema->active->paths--;
    return result;
}

static int test_valid(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
{
    int result = validate(schema, rule, node, abortable);

    if (!abortable && (result == SCHEMA_INVALID))
    {
        return ~SCHEMA_INVALID;
    }
    return result;
}

static int test_child(const schema_t *schema, const json_t *rule, const json_t *parent,
    unsigned child, int abortable)
{
    if (schema->active->paths++ < MAX_PATHS)
    {
        schema->active->path[schema->active->paths - 1] = child;
    }

    int result = validate(schema, rule, parent->child[child], abortable);

    if (!abortable && (result == SCHEMA_INVALID))
    {
        result = ~SCHEMA_INVALID;
    }
    schema->active->paths--;
    return result;
}

static int test_properties(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
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
        if (!(rule->child[i]->type & (JSON_OBJECT | JSON_FALSE)))
        {
            return SCHEMA_ERROR;
        }

        unsigned index = json_index(node, rule->child[i]->key);

        if (index == JSON_NOT_FOUND)
        {
            continue;
        }

        int test;

        if (rule->child[i]->type == JSON_OBJECT)
        {
            test = test_child(schema, rule->child[i], node, index, abortable);
        }
        else
        {
            test = test_false(schema, rule->child[i], node, index, abortable);
        }
        if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
        {
            return test;
        }
        result &= test;
    }
    return ~result;
}

static int test_pattern_properties(const schema_t *schema, const json_t *rule,
    const json_t *node, int abortable)
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
        if (!(rule->child[i]->type & (JSON_OBJECT | JSON_FALSE)))
        {
            return SCHEMA_ERROR;
        }
        for (unsigned j = 0; j < node->size; j++)
        {
            if (!test_regex(node->child[j]->key, rule->child[i]->key))
            {
                continue;
            }

            int test;

            if (rule->child[i]->type == JSON_FALSE)
            {
                const json_t note =
                {
                    .key = rule->key,
                    .child = (json_t *[]){rule->child[i]},
                    .size = 1,
                    .type = JSON_OBJECT
                };

                test = test_false(schema, &note, node, j, abortable);
            }
            else
            {
                test = test_child(schema, rule->child[i], node, j, abortable);
            }
            if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
            {
                return test;
            }
            result &= test;
        }
    }
    return ~result;
}

static int test_additional_properties(const schema_t *schema, const json_t *parent,
    const json_t *rule, const json_t *node, int abortable)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
        case JSON_FALSE:
            break;
        case JSON_TRUE:
            return SCHEMA_VALID;
        default:
            return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    const json_t *properties = json_find(parent, "properties");
    const json_t *patterns = json_find(parent, "patternProperties");
    unsigned patterns_size = json_properties(patterns);
    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < node->size; i++)
    {
        if (json_find(properties, node->child[i]->key))
        {
            continue;
        }
        for (unsigned j = 0; j < patterns_size; j++)
        {
            if (test_regex(node->child[i]->key, patterns->child[j]->key))
            {
                goto jump;
            }
        }

        int test;

        if (rule->type == JSON_OBJECT)
        {
            test = test_child(schema, rule, node, i, abortable);
        }
        else
        {
            test = test_false(schema, rule, node, i, abortable);
        }
        if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
        {
            return test;
        }
        result &= test;
        jump:;
    }
    return ~result;
}

static int test_property_names(const schema_t *schema, const json_t *rule,
    const json_t *node, int abortable)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
            break;
        case JSON_FALSE:
            return !json_properties(node);
        case JSON_TRUE:
            return SCHEMA_VALID;
        default:
            return SCHEMA_ERROR;
    }
    if (node->type != JSON_OBJECT)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < node->size; i++)
    {
        const json_t name =
        {
            .key = "propertyName",
            .string = node->child[i]->key,
            .type = JSON_STRING
        };

        int test = test_valid(schema, rule, &name, abortable);

        if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
        {
            return test;
        }
        result &= test;
    }
    return ~result;
}

static int test_required(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
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
        if (json_find(node, rule->child[i]->string))
        {
            continue;
        }

        const json_t note =
        {
            .key = rule->key,
            .string = rule->child[i]->string,
            .type = JSON_STRING
        };

        int test = test_abort(schema, &note, node, abortable);

        if (test != SCHEMA_INVALID)
        {
            return test;
        }
        result &= test;
    }
    return ~result;
}

static int test_dependencies(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
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
        if (!(rule->child[i]->type & JSON_ITERABLE))
        {
            return SCHEMA_ERROR;
        }
        if (!json_find(node, rule->child[i]->key))
        {
            continue;
        }
        if (rule->child[i]->type == JSON_OBJECT)
        {
            int test = validate(schema, rule->child[i], node, abortable);

            if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
            {
                return test;
            }
            result &= test;
        }
        else
        {
            const json_t *properties = rule->child[i];

            for (unsigned j = 0; j < properties->size; j++)
            {
                if (properties->child[j]->type != JSON_STRING)
                {
                    return SCHEMA_ERROR;
                }
                if (json_find(node, properties->child[j]->string))
                {
                    continue;
                }

                const json_t note =
                {
                    .key = rule->key,
                    .child = (json_t *[]){rule->child[i]},
                    .size = 1,
                    .type = JSON_OBJECT
                };

                int test = test_abort(schema, &note, node, abortable);

                if (test != SCHEMA_INVALID)
                {
                    return test;
                }
                result &= test;
            }
        }
    }
    return ~result;
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

static int test_items(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
        case JSON_ARRAY:
            break;
        case JSON_FALSE:
            return !json_items(node);
        case JSON_TRUE:
            return SCHEMA_VALID;
        default:
            return SCHEMA_ERROR;
    }
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }

    int result = SCHEMA_VALID;

    if (rule->type == JSON_OBJECT)
    {
        for (unsigned i = 0; i < node->size; i++)
        {
            int test = test_child(schema, rule, node, i, abortable);

            if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
            {
                return test;
            }
            result &= test;
        }
    }
    else
    {
        for (unsigned i = 0; (i < rule->size) && (i < node->size); i++)
        {
            int test = SCHEMA_VALID;

            if (rule->child[i]->type == JSON_OBJECT)
            {
                test = test_child(schema, rule->child[i], node, i, abortable);
            }
            else if (rule->child[i]->type == JSON_FALSE)
            {
                test = test_false(schema, rule->child[i], node, i, abortable);
            }
            else if (rule->child[i]->type != JSON_TRUE)
            {
                return SCHEMA_ERROR;
            }
            if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
            {
                return test;
            }
            result &= test;
        }
    }
    return ~result;
}

static int test_additional_items(const schema_t *schema, const json_t *parent,
    const json_t *rule, const json_t *node, int abortable)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
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

    const json_t *items = json_find(parent, "items");

    if (json_items(items) == 0)
    {
        return SCHEMA_VALID;
    }
    if (rule->type == JSON_FALSE)
    {
        return node->size <= items->size;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = items->size; i < node->size; i++)
    {
        int test = test_child(schema, rule, node, i, abortable);

        if ((test != SCHEMA_VALID) && (test != SCHEMA_INVALID))
        {
            return test;
        }
        result &= test;
    }
    return ~result;
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

static int test_contains(const schema_t *schema, const json_t *rule, const json_t *node)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
            break;
        case JSON_FALSE:
            return !json_items(node);
        case JSON_TRUE:
            return SCHEMA_VALID;
        default:
            return SCHEMA_ERROR;
    }
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }
    for (unsigned i = 0; i < node->size; i++)
    {
        int test = validate(schema, rule, node->child[i], NOT_ABORTABLE);

        if (test == SCHEMA_ABORT)
        {
            return SCHEMA_ABORT;
        }
        if (test == SCHEMA_VALID)
        {
            return SCHEMA_VALID;
        }
    }
    return SCHEMA_INVALID;
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

static int test_format(const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_STRING)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_STRING)
    {
        return SCHEMA_VALID;
    }
    return test_match(node->string, rule->string);
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

static int test_x_mask(const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_STRING)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_STRING)
    {
        return SCHEMA_VALID;
    }
    return test_mask(node->string, rule->string) != NULL;
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

static int test_minimum(const json_t *rule, const json_t *node)
{
    if (!(rule->type & JSON_NUMBER))
    {
        return SCHEMA_ERROR;
    }
    if (!(node->type & JSON_NUMBER))
    {
        return SCHEMA_VALID;
    }
    return node->number >= rule->number;
}

static int test_maximum(const json_t *rule, const json_t *node)
{
    if (!(rule->type & JSON_NUMBER))
    {
        return SCHEMA_ERROR;
    }
    if (!(node->type & JSON_NUMBER))
    {
        return SCHEMA_VALID;
    }
    return node->number <= rule->number;
}

static int test_exclusive_minimum(const json_t *rule, const json_t *node)
{
    if (!(rule->type & JSON_NUMBER))
    {
        return SCHEMA_ERROR;
    }
    if (!(node->type & JSON_NUMBER))
    {
        return SCHEMA_VALID;
    }
    return node->number > rule->number;
}

static int test_exclusive_maximum(const json_t *rule, const json_t *node)
{
    if (!(rule->type & JSON_NUMBER))
    {
        return SCHEMA_ERROR;
    }
    if (!(node->type & JSON_NUMBER))
    {
        return SCHEMA_VALID;
    }
    return node->number < rule->number;
}

static int test_const(const json_t *rule, const json_t *node)
{
    return json_equal(rule, node);
}

static int test_enum(const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_ARRAY) || (rule->size == 0))
    {
        return SCHEMA_ERROR;
    }
    return json_locate(rule, node) != NULL;
}

static unsigned add_type(const char *type, unsigned *mask)
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
            *mask |= 1u << item;
            return 1;
        }
    }
    return 0;
}

static int test_type(const json_t *rule, const json_t *node)
{
    unsigned mask = 0;

    switch (rule->type)
    {
        case JSON_STRING:
            if (!add_type(rule->string, &mask))
            {
                return SCHEMA_ERROR;
            }
            break;
        case JSON_ARRAY:
            for (unsigned i = 0; i < rule->size; i++)
            {
                if (!add_type(json_text(rule->child[i]), &mask))
                {
                    return SCHEMA_ERROR;
                }
            }
            break;
        default:
            return SCHEMA_ERROR;
    }
    return
        /* Reduce JSON_FALSE and JSON_NULL in order to match 'type' offsets */
        (mask & (node->type < JSON_FALSE ? node->type : node->type >> 1u)) ||
        /* 'integer' validates as true when 'type' is 'number' */
        ((mask & JSON_REAL) && (node->type == JSON_INTEGER));
}

static int test_ref(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
{
    if (rule->type != JSON_STRING)
    {
        return SCHEMA_ERROR;
    }

    const char *ref = rule->string;

    if (ref[0] == '#')
    {
        if (ref[1] == '/')
        {
            rule = json_pointer(schema->rule, ref + 1);
        }
        else if (ref[1] == '\0')
        {
            rule = schema->rule;
        }
    }
    else if ((ref[0] == '/') && (schema->map != NULL))
    {
        size_t length = strcspn(ref + 1, "/") + 1;

        rule = json_pointer(map_prefix(schema->map, ref, length), ref + length);
    }
    else
    {
        rule = NULL;
    }
    if ((rule == NULL) || (rule->type != JSON_OBJECT))
    {
        return SCHEMA_ERROR;
    }
    if (++schema->active->refs >= MAX_REFS)
    {
        const json_t note =
        {
            .key = "maxRefs",
            .number = MAX_REFS,
            .type = JSON_INTEGER
        };

        raise_error(schema, &note, node);
        return SCHEMA_ABORT;
    }

    int result = validate(schema, rule, node, abortable);

    schema->active->refs--;
    return ~result;
}

static int test_not(const schema_t *schema, const json_t *rule, const json_t *node)
{
    switch (rule->type)
    {
        case JSON_OBJECT:
            break;
        case JSON_FALSE:
            return SCHEMA_VALID;
        case JSON_TRUE:
            return SCHEMA_INVALID;
        default:
            return SCHEMA_ERROR;
    }

    int result = validate(schema, rule, node, NOT_ABORTABLE);

    if ((result != SCHEMA_VALID) && (result != SCHEMA_INVALID))
    {
        return result;
    }
    return !result;
}

static int test_any_of(const schema_t *schema, const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_ARRAY)
    {
        return SCHEMA_ERROR;
    }
    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }

        int test = validate(schema, rule->child[i], node, NOT_ABORTABLE);

        if (test != SCHEMA_INVALID)
        {
            return test;
        }
    }
    return SCHEMA_INVALID;
}

static int test_one_of(const schema_t *schema, const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_ARRAY)
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

        int test = validate(schema, rule->child[i], node, NOT_ABORTABLE);

        if (test == SCHEMA_ABORT)
        {
            return SCHEMA_ABORT;
        }
        if ((test == SCHEMA_VALID) && (count++ == 1))
        {
            return SCHEMA_INVALID;
        }
    }
    return count == 1;
}

static int test_all_of(const schema_t *schema, const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_ARRAY)
    {
        return SCHEMA_ERROR;
    }
    for (unsigned i = 0; i < rule->size; i++)
    {
        if (rule->child[i]->type != JSON_OBJECT)
        {
            return SCHEMA_ERROR;
        }

        int test = validate(schema, rule->child[i], node, NOT_ABORTABLE);

        if (test != SCHEMA_VALID)
        {
            return test;
        }
    }
    return SCHEMA_VALID;
}

static int test_if(const schema_t *schema, const json_t *parent, unsigned *child,
    const json_t *node)
{
    unsigned index = *child;
    const json_t *rule = parent->child[index];

    if (!(rule->type & (JSON_OBJECT | JSON_BOOLEAN)))
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
    if (!(next->type & (JSON_OBJECT | JSON_BOOLEAN)))
    {
        *child += 1; // Move to next branch
        return SCHEMA_ERROR;
    }

    int result = rule->type == JSON_OBJECT
        ? validate(schema, rule, node, NOT_ABORTABLE)
        : rule->type == JSON_TRUE;

    if (result == SCHEMA_ABORT)
    {
        return SCHEMA_ABORT;
    }
    if (result == branch)
    {
        *child += 1;
    }
    return SCHEMA_VALID;
}

static int test_branch(const schema_t *schema, const json_t *parent, unsigned *child,
    const json_t *node, int abortable)
{
    unsigned index = *child;
    const json_t *rule = parent->child[index];
    int result = SCHEMA_VALID;

    if (rule->type == JSON_OBJECT)
    {
        result = validate(schema, rule, node, abortable);
    }
    else if (rule->type == JSON_FALSE)
    {
        result = abortable && abort_on_failure(schema, rule, node)
            ? SCHEMA_ERROR
            : SCHEMA_INVALID;
    }
    else if (rule->type != JSON_TRUE)
    {
        return SCHEMA_ERROR;
    }
    if (result == SCHEMA_ABORT)
    {
        return SCHEMA_ABORT;
    }
    if (parent->size > index + 1)
    {
        // Skip next branch if is 'else' or 'then'
        if (!strcmp(parent->child[index + 1]->key, "else") ||
            !strcmp(parent->child[index + 1]->key, "then"))
        {
            *child += 1;
        }
    }
    return ~result;
}

static int test_x_notify(const schema_t *schema, const json_t *rule, const json_t *node)
{
    if ((rule->type != JSON_OBJECT) || abort_on_notify(schema, rule, node))
    {
        return SCHEMA_ABORT;
    }
    return SCHEMA_VALID;
}

static int validate(const schema_t *schema, const json_t *rule, const json_t *node,
    int abortable)
{
    int result = SCHEMA_VALID;

    for (unsigned i = 0; i < rule->size; i++)
    {
        int test = get_test(rule->child[i]);

        switch (test)
        {
            case SCHEMA_VALID:
                continue;
            case SCHEMA_WARNING:
                if (abort_on_warning(schema, rule->child[i], node))
                {
                    return SCHEMA_ABORT;
                }
                continue;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ABORT;
            // Validate object related tests
            case SCHEMA_PROPERTIES:
                test = test_properties(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_PATTERN_PROPERTIES:
                test = test_pattern_properties(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_ADDITIONAL_PROPERTIES:
                test = test_additional_properties(schema, rule, rule->child[i], node, abortable);
                break;
            case SCHEMA_PROPERTY_NAMES:
                test = test_property_names(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_REQUIRED:
                test = test_required(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_DEPENDENCIES:
                test = test_dependencies(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_MIN_PROPERTIES:
                test = test_min_properties(rule->child[i], node);
                break;
            case SCHEMA_MAX_PROPERTIES:
                test = test_max_properties(rule->child[i], node);
                break;
            // Validate array related tests
            case SCHEMA_ITEMS:
                test = test_items(schema, rule->child[i], node, abortable);
                break;
            case SCHEMA_ADDITIONAL_ITEMS:
                test = test_additional_items(schema, rule, rule->child[i], node, abortable);
                break;
            case SCHEMA_UNIQUE_ITEMS:
                test = test_unique_items(rule->child[i], node);
                break;
            case SCHEMA_CONTAINS:
                test = test_contains(schema, rule->child[i], node);
                break;
            case SCHEMA_MIN_ITEMS:
                test = test_min_items(rule->child[i], node);
                break;
            case SCHEMA_MAX_ITEMS:
                test = test_max_items(rule->child[i], node);
                break;
            // Validate string related tests
            case SCHEMA_FORMAT:
                test = test_format(rule->child[i], node);
                break;
            case SCHEMA_PATTERN:
                test = test_pattern(rule->child[i], node);
                break;
            case SCHEMA_X_MASK:
                test = test_x_mask(rule->child[i], node);
                break;
            case SCHEMA_MIN_LENGTH:
                test = test_min_length(rule->child[i], node);
                break;
            case SCHEMA_MAX_LENGTH:
                test = test_max_length(rule->child[i], node);
                break;
            // Validate number related tests
            case SCHEMA_MULTIPLE_OF:
                test = test_multiple_of(rule->child[i], node);
                break;
            case SCHEMA_MINIMUM:
                test = test_minimum(rule->child[i], node);
                break;
            case SCHEMA_MAXIMUM:
                test = test_maximum(rule->child[i], node);
                break;
            case SCHEMA_EXCLUSIVE_MINIMUM:
                test = test_exclusive_minimum(rule->child[i], node);
                break;
            case SCHEMA_EXCLUSIVE_MAXIMUM:
                test = test_exclusive_maximum(rule->child[i], node);
                break;
            // Validate global tests
            case SCHEMA_CONST:
                test = test_const(rule->child[i], node);
                break;
            case SCHEMA_ENUM:
                test = test_enum(rule->child[i], node);
                break;
            case SCHEMA_TYPE:
                test = test_type(rule->child[i], node);
                break;
            // Validate references
            case SCHEMA_REF:
                test = test_ref(schema, rule->child[i], node, abortable);
                break;
            // Validate special case 'not'
            case SCHEMA_NOT:
                test = test_not(schema, rule->child[i], node);
                break;
            // Validate combinators tests
            case SCHEMA_ANY_OF:
                test = test_any_of(schema, rule->child[i], node);
                break;
            case SCHEMA_ONE_OF:
                test = test_one_of(schema, rule->child[i], node);
                break;
            case SCHEMA_ALL_OF:
                test = test_all_of(schema, rule->child[i], node);
                break;
            // Validate logical tests
            case SCHEMA_IF:
                test = test_if(schema, rule, &i, node);
                break;
            case SCHEMA_THEN:
            case SCHEMA_ELSE:
                test = test_branch(schema, rule, &i, node, abortable);
                break;
            // Notification to user-callback (extension)
            case SCHEMA_X_NOTIFY:
                test = test_x_notify(schema, rule->child[i], node);
                break;
            // Rule not handled (shouldn't get here)
            default:
                assert(0 && "Unhandled test case");
                test = SCHEMA_ERROR;
                break;
        }
        switch (test)
        {
            case SCHEMA_INVALID:
                if (!abortable)
                {
                    return SCHEMA_INVALID;
                }
                if (abort_on_failure(schema, rule->child[i], node))
                {
                    return SCHEMA_ABORT;
                }
                result = SCHEMA_INVALID;
                break;
            case ~SCHEMA_INVALID:
                if (!abortable)
                {
                    return SCHEMA_INVALID;
                }
                result = SCHEMA_INVALID;
                break;
            case SCHEMA_ABORT:
                return SCHEMA_ABORT;
            case SCHEMA_ERROR:
                raise_error(schema, rule->child[i], node);
                return SCHEMA_ABORT;
        }
    }
    return result;
}

int json_validate(const json_t *rule, const json_t *node, const map_t *map,
    json_validate_callback callback, void *data)
{
    struct tracker active = {0};
    const schema_t schema =
    {
        .rule = rule, .node = node, .map = map,
        .callback = callback, .data = data,
        .active = &active
    };

    if ((rule == NULL) || !(rule->type & (JSON_OBJECT | JSON_TRUE)) || (node == NULL))
    {
        return SCHEMA_INVALID;
    }
    return validate(&schema, rule, node, ABORTABLE) == SCHEMA_VALID;
}

