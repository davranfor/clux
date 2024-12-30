/*!
 *  \brief     clux - json and json-schema library for C
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
#include "json_pointer.h"
#include "json_schema.h"

#define PATH_MAX_SIZE 256
#define MAX_ACTIVE_PATHS 16
#define MAX_ACTIVE_REFS 128

enum {NOT_ABORTABLE, ABORTABLE};

struct active
{
    const json_t *path[MAX_ACTIVE_PATHS];
    size_t item[MAX_ACTIVE_PATHS];
    int paths;
    int refs;
};

typedef struct
{
    // Schema root
    const json_t *root;
    // User data
    json_validate_callback callback;
    void *data;
    // Paths and references
    struct active *active;
} schema_t;


static int validate(const schema_t *, const json_t *, const json_t *, int);

static int validate_schema(const json_t *rule, const json_t *node,
    json_validate_callback callback, void *data, int abortable)
{
    struct active active =
    {
        .path[0] = node,
        .paths = 1
    };
    const schema_t schema =
    {
        .root = rule,
        .callback = callback,
        .data = data,
        .active = &active
    };

    if ((rule == NULL) || (rule->type != JSON_OBJECT) || (node == NULL))
    {
        return -1;
    }
    return validate(&schema, rule, node, abortable);
}

/* Writes the current path and notifies an event to the user-defined callback */
static int notify_event(const schema_t *schema,
    const json_t *rule, const json_t *node, int type)
{
    char path[PATH_MAX_SIZE] = "/";
    char *end = path;

    for (int i = 1; (i < schema->active->paths) && (i < MAX_ACTIVE_PATHS); i++)
    {
        size_t size = sizeof(path) - (size_t)(end - path);
        const char *pointer = end;

        end += schema->active->path[i]->key != NULL
            ? json_pointer_put_key(end, size, schema->active->path[i]->key)
            : json_pointer_put_index(end, size, schema->active->item[i]);
        if (end == pointer)
        {
            // Doesn't fit, try adding '...' to the path 
            end += json_pointer_put_key(end, size, "...");
        }
    }

    const json_schema_event_t event =
    {
        .type = type, .path = path, .node = node, .rule = rule
    };

    return schema->callback(&event, schema->data);
}

static int notify(const schema_t *schema,
    const json_t *rule, const json_t *node, int event)
{
    return schema->callback
        ? notify_event(schema, rule, node, event)
        : event == JSON_SCHEMA_FAILURE
            ? JSON_SCHEMA_ABORT
            : JSON_SCHEMA_CONTINUE;
}

static int abort_on_warning(const schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_WARNING) == JSON_SCHEMA_ABORT;
}

static int abort_on_failure(const schema_t *schema,
    const json_t *rule, const json_t *node)
{
    return notify(schema, rule, node, JSON_SCHEMA_FAILURE) == JSON_SCHEMA_ABORT;
}

static void raise_error(const schema_t *schema,
    const json_t *rule, const json_t *node)
{
    notify(schema, rule, node, JSON_SCHEMA_ABORTED);
}

/**
 * Writes an event to a provided buffer
 * event->node: Inner nodes are omitted
 * event->rule: Cropped to the first ':' before max_length
 */
char *json_schema_write_event(const json_schema_event_t *event, buffer_t *buffer)
{
    static const char *events[] =
    {
        "Warning. Unknown schema rule",
        "Failure. Doesn't validate against schema rule",
        "Aborted. Malformed schema",
    };

    buffer_print(buffer, "\ntype: %s", events[event->type]);
    buffer_print(buffer, "\npath: %s", event->path);
    buffer_write(buffer, "\nnode: ");
    if (!json_is_scalar(event->node))
    {
        const char *type = json_is_object(event->node) ? "Object" : "Array";
        const char *name = json_name(event->node);
        unsigned size = event->node->size;

        if (event->node->key != NULL)
        {
            buffer_print(buffer, "%s '%s' (%u elements)", type, name, size);
        }
        else
        {
            buffer_print(buffer, "%s (%u elements)", type, size);
        }
    }
    else
    {
        json_buffer_write(buffer, event->node, 0);
    }
    buffer_write(buffer, "\nrule: ");

    size_t length = buffer->length;
    size_t max_length = 192;

    json_buffer_write(buffer, event->rule, 0);
    if (buffer->length > length + max_length)
    {
        length += max_length;
        while (buffer->text[length] != ':')
        {
            length--;
        }
        buffer->length = length;
        buffer->text[length] = '\0';
        buffer_write(buffer, ": ...");
    }
    return buffer_write(buffer, "\n");
}

static map_t *map;

void json_schema_set_map(map_t *pmap)
{
    map = pmap;
}

map_t *json_schema_get_map(void)
{
    return map;
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

typedef struct test { const char *key; struct test *next; } test_t;

/**
 * ------------------------------------------------------------------
 * X macro indexing enum and array of tests
 * Schema (Draft 07) keywords
 * https://json-schema.org/draft-07/schema
 * ------------------------------------------------------------------
 */
#define TEST(_)                                                     \
    _(SCHEMA_ADDITIONAL_ITEMS,          "additionalItems")          \
    _(SCHEMA_ADDITIONAL_PROPERTIES,     "additionalProperties")     \
    _(SCHEMA_ALL_OF,                    "allOf")                    \
    _(SCHEMA_ANY_OF,                    "anyOf")                    \
    _(SCHEMA_COMMENT,                   "$comment")                 \
    _(SCHEMA_CONST,                     "const")                    \
    _(SCHEMA_CONTENT_ENCODING,          "contentEncoding")          \
    _(SCHEMA_CONTENT_MEDIA_TYPE,        "contentMediaType")         \
    _(SCHEMA_CONTAINS,                  "contains")                 \
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
    _(SCHEMA_WRITE_ONLY,                "writeOnly")

#define TEST_ENUM(a, b) a,
enum
{
    SCHEMA_ERROR = -1, SCHEMA_INVALID, SCHEMA_VALID,
    SCHEMA_WARNING, SCHEMA_FAILURE, SCHEMA_ABORTED,
    DEFS, TEST(TEST_ENUM) NTESTS
};

#define TEST_KEY(a, b) {.key = b},
static test_t tests[] = {TEST(TEST_KEY)};

enum {TABLE_SIZE = NTESTS - DEFS - 1};
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
            return (rule->type == JSON_TRUE) || (rule->type == JSON_FALSE)
                ? SCHEMA_VALID
                : SCHEMA_ERROR;
        // Rules that need to be tested 
        default:
            return test;
    }
}

static int test_property(const schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (schema->active->paths++ < MAX_ACTIVE_PATHS)
    {
        schema->active->path[schema->active->paths - 1] = node;
    }

    int result = validate(schema, rule, node, abortable);

    schema->active->paths--;
    return result;
}

static int test_properties(const schema_t *schema,
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
        switch (test_property(schema, rule->child[i], property, abortable))
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
        }
    }
    return result;
}

static int test_pattern_properties(const schema_t *schema,
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
        for (unsigned j = 0; j < node->size; j++)
        {
            if (!test_regex(node->child[j]->key, rule->child[i]->key))
            {
                continue;
            };
            if (rule->child[i]->type == JSON_FALSE)
            {
                if (!abortable)
                {
                    return SCHEMA_FAILURE;
                }

                const json_t note =
                {
                    .key = rule->key,
                    .child = (json_t *[]){rule->child[i]},
                    .size = 1,
                    .type = JSON_OBJECT
                };

                if (abort_on_failure(schema, &note, node->child[j]))
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
            switch (test_property(schema, rule->child[i], node->child[j], abortable))
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
            }
        }
    }
    return result;
}

static int test_additional_properties(const schema_t *schema,
    const json_t *parent, const json_t *rule, const json_t *node, int abortable)
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

        int found = 0;

        for (unsigned j = 0; j < patterns_size; j++)
        {
            if (test_regex(node->child[i]->key, patterns->child[j]->key))
            {
                found = 1;
                break;
            };
        }
        if (found == 1)
        {
            continue;
        }
        switch (rule->type)
        {
            case JSON_FALSE:
                if (!abortable)
                {
                    return SCHEMA_FAILURE;
                }
                if (abort_on_failure(schema, rule, node->child[i]))
                {
                    return SCHEMA_ABORTED;
                }
                result = SCHEMA_FAILURE;
                break;
            case JSON_OBJECT:
                switch (test_property(schema, rule, node->child[i], abortable))
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
                }
                break;
        }
    }
    return result;
}

static int test_property_names(const schema_t *schema,
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

    for (unsigned i = 0; i < node->size; i++)
    {
        const json_t name =
        {
            .key = "propertyName",
            .string = node->child[i]->key,
            .type = JSON_STRING
        };

        switch (validate(schema, rule, &name, abortable))
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
        }
    }
    return result;
}

static int test_required(const schema_t *schema,
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
        if (json_find(node, rule->child[i]->string))
        {
            continue;
        }
        if (abortable)
        {
            const json_t note =
            {
                .key = rule->key,
                .string = rule->child[i]->string,
                .type = JSON_STRING
            };

            if (abort_on_failure(schema, &note, node))
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
    return result;
}

static int test_dependencies(const schema_t *schema,
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
        if (!json_is_iterable(rule->child[i]))
        {
            return SCHEMA_ERROR;
        }
        if (!json_find(node, rule->child[i]->key))
        {
            continue;
        }
        if (rule->child[i]->type == JSON_OBJECT)
        {
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
            }
        }
        else
        {
            const json_t *keys = rule->child[i];

            for (unsigned j = 0; j < keys->size; j++)
            {
                if (keys->child[j]->type != JSON_STRING)
                {
                    return SCHEMA_ERROR;
                }
                if (json_find(node, keys->child[j]->string))
                {
                    continue;
                }
                if (abortable)
                {
                    const json_t note =
                    {
                        .key = rule->key,
                        .child = (json_t *[]){rule->child[i]},
                        .size = 1,
                        .type = JSON_OBJECT
                    };

                    if (abort_on_failure(schema, &note, node))
                    {
                        return SCHEMA_ABORTED;
                    }
                    result = SCHEMA_FAILURE;
                    continue;
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

static int test_item(const schema_t *schema,
    const json_t *rule, const json_t *parent, unsigned child, int abortable)
{
    if (schema->active->paths++ < MAX_ACTIVE_PATHS)
    {
        schema->active->path[schema->active->paths - 1] = parent->child[child];
        schema->active->item[schema->active->paths - 1] = child;
    }

    int result = validate(schema, rule, parent->child[child], abortable);

    schema->active->paths--;
    return result;
}

static int test_items(const schema_t *schema,
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

    int result = SCHEMA_VALID;

    if (rule->type == JSON_OBJECT)
    {
        for (unsigned i = 0; i < node->size; i++)
        {
            switch (test_item(schema, rule, node, i, abortable))
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
            }
        }
    }
    else
    {
        for (unsigned i = 0; (i < rule->size) && (i < node->size); i++)
        {
            if (rule->child[i]->type != JSON_OBJECT)
            {
                return SCHEMA_ERROR;
            }
            switch (test_item(schema, rule->child[i], node, i, abortable))
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
            }
        }
    }
    return result;
}

static int test_additional_items(const schema_t *schema,
    const json_t *parent, const json_t *rule, const json_t *node, int abortable)
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

    if (json_is_object(items))
    {
        return SCHEMA_VALID;
    }

    unsigned offset = json_size(items);

    if (offset == 0)
    {
        return SCHEMA_VALID;
    }
    if (rule->type == JSON_FALSE)
    {
        return node->size <= offset;
    }

    int result = SCHEMA_VALID;

    for (unsigned i = offset; i < node->size; i++)
    {
        switch (test_item(schema, rule, node, i, abortable))
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
        }
    }
    return result;
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

static int test_contains(const schema_t *schema,
    const json_t *rule, const json_t *node)
{
    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }
    if (node->type != JSON_ARRAY)
    {
        return SCHEMA_VALID;
    }
    for (unsigned i = 0; i < node->size; i++)
    {
        switch (test_item(schema, rule, node, i, NOT_ABORTABLE))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_VALID:
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
    if ((node->type != JSON_STRING) || (node->string[0] == 0))
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

    unsigned type = node->type;

    /* Reduce JSON_FALSE and JSON_NULL in order to match 'type' offsets */
    return (mask & (1u << (type < JSON_FALSE ? type : type - 1))) ||
          /* 'integer' validates as true when type is 'number' */
          ((mask & (1u << JSON_REAL)) && (type == JSON_INTEGER));
}

static int test_external_ref(const schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
{
    if (++schema->active->refs >= MAX_ACTIVE_REFS)
    {
        const json_t note =
        {
            .key = "maxActiveRefs",
            .number = MAX_ACTIVE_REFS,
            .type = JSON_INTEGER
        };

        raise_error(schema, &note, node);
        return SCHEMA_ABORTED;
    }

    int result = validate_schema(
        rule, node, schema->callback, schema->data, abortable
    );

    schema->active->refs--;
    switch (result)
    {
        case SCHEMA_ERROR:
            return SCHEMA_ABORTED;
        case SCHEMA_INVALID:
            return SCHEMA_FAILURE;
        default:
            return SCHEMA_VALID;
    }
}

static int test_ref(const schema_t *schema,
    const json_t *rule, const json_t *node, int abortable)
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
            rule = json_pointer(schema->root, ref + 1);
        }
        else if (ref[1] == '\0')
        {
            rule = schema->root;
        }
    }
    else
    {
        rule = map_search(map, ref);
        return test_external_ref(schema, rule, node, abortable);
    }
    if ((rule == NULL) || (rule->type != JSON_OBJECT))
    {
        return SCHEMA_ERROR;
    }
    if (++schema->active->refs >= MAX_ACTIVE_REFS)
    {
        const json_t note =
        {
            .key = "maxActiveRefs",
            .number = MAX_ACTIVE_REFS,
            .type = JSON_INTEGER
        };

        raise_error(schema, &note, node);
        return SCHEMA_ABORTED;
    }

    int result = validate(schema, rule, node, abortable);

    schema->active->refs--;
    switch (result)
    {
        case SCHEMA_ERROR:
            return SCHEMA_ABORTED;
        case SCHEMA_INVALID:
            return SCHEMA_FAILURE;
        default:
            return SCHEMA_VALID;
    }
}

static int test_not(const schema_t *schema,
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
 
    int result = validate(schema, rule, node, NOT_ABORTABLE);

    if (result == SCHEMA_ERROR)
    {
        return SCHEMA_ABORTED;
    }
    return !result;
}

static int test_any_of(const schema_t *schema,
    const json_t *rule, const json_t *node)
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
        switch (validate(schema, rule->child[i], node, NOT_ABORTABLE))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_VALID:
                return SCHEMA_VALID;
        }
    }
    return SCHEMA_INVALID;
}

static int test_one_of(const schema_t *schema,
    const json_t *rule, const json_t *node)
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
        switch (validate(schema, rule->child[i], node, NOT_ABORTABLE))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_VALID:
                if (count++ == 1)
                {
                    return SCHEMA_INVALID;
                }
                break;
        }
    }
    return count == 1;
}

static int test_all_of(const schema_t *schema,
    const json_t *rule, const json_t *node)
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
        switch (validate(schema, rule->child[i], node, NOT_ABORTABLE))
        {
            case SCHEMA_ERROR:
                return SCHEMA_ABORTED;
            case SCHEMA_INVALID:
                return SCHEMA_INVALID;
        }
    }
    return SCHEMA_VALID;
}

static int test_if(const schema_t *schema,
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

    int result = validate(schema, rule, node, NOT_ABORTABLE);

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

static int test_branch(const schema_t *schema,
    const json_t *parent, unsigned *child, const json_t *node, int abortable)
{
    unsigned index = *child;
    const json_t *rule = parent->child[index];

    if (rule->type != JSON_OBJECT)
    {
        return SCHEMA_ERROR;
    }

    int result = validate(schema, rule, node, abortable);

    switch (result)
    {
        case SCHEMA_ERROR:
            return SCHEMA_ABORTED;
        case SCHEMA_INVALID:
            result = SCHEMA_FAILURE;
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
    return result;
}

static int validate(const schema_t *schema,
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
            // Rule not handled (shouldn't get here)
            default:
                assert(0 && "Unhandled test case");
                test = SCHEMA_ERROR;
                break;
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
        }
    }
    return result;
}

int json_validate(const json_t *node, const json_t *rule,
    json_validate_callback callback, void *data)
{
    return validate_schema(rule, node, callback, data, ABORTABLE) > 0;
}

