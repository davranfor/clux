/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_H
#define JSON_H

#include <stdio.h>

enum json_type
{
    JSON_UNDEFINED,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_BOOLEAN,
    JSON_NULL,
};

enum json_encode
{
    JSON_UTF8,
    JSON_ASCII,
};

typedef struct json json;
typedef struct json_map json_map;
typedef struct {int line, column;} json_error;
typedef int (*json_walk_callback)(const json *, int, void *);
typedef int (*json_sort_callback)(const json *, const json *);
typedef int (*json_map_walk_callback)(json *, size_t, void *);

// ============================================================================
// Parser
// ============================================================================
json *json_parse(const char *, json_error *);
json *json_parse_file(const char *, json_error *);
void json_print_error(const json_error *);
// ============================================================================
// Reader
// ============================================================================
enum json_type json_type(const json *);
const char *json_type_name(const json *);
const char *json_key(const json *);
const char *json_name(const json *);
const char *json_string(const json *);
double json_number(const json *);
int json_boolean(const json *);
int json_is_any(const json *);
int json_is_iterable(const json *);
int json_is_scalar(const json *);
int json_is_object(const json *);
int json_is_array(const json *);
int json_is_string(const json *);
int json_is_integer(const json *);
int json_is_unsigned(const json *);
int json_is_real(const json *);
int json_is_number(const json *);
int json_is_boolean(const json *);
int json_is_true(const json *);
int json_is_false(const json *);
int json_is_null(const json *);
json *json_self(const json *);
json *json_root(const json *);
json *json_parent(const json *);
json *json_child(const json *);
json *json_head(const json *);
json *json_prev(const json *);
json *json_next(const json *);
json *json_tail(const json *);
json *json_at(const json *, size_t);
json *json_find(const json *, const char *);
json *json_find_prev(const json *, const char *);
json *json_find_next(const json *, const char *);
size_t json_length(const json *);
size_t json_size(const json *);
size_t json_offset(const json *);
size_t json_height(const json *);
size_t json_depth(const json *);
int json_compare(const json *, const json *);
int json_equal(const json *, const json *);
int json_walk(const json *, json_walk_callback, void *);
// ============================================================================
// Writer
// ============================================================================
json *json_new_object(const char *);
json *json_new_array(const char *);
json *json_new_format(const char *, const char *, ...)
    __attribute__ ((format (printf, 2, 3)));
json *json_new_string(const char *, const char *);
json *json_new_integer(const char *, double);
json *json_new_real(const char *, double);
json *json_new_boolean(const char *, int);
json *json_new_null(const char *);
json *json_set_name(json *, const char *);
json *json_set_object(json *);
json *json_set_array(json *);
json *json_set_format(json *, const char *, ...)
    __attribute__ ((format (printf, 2, 3)));
json *json_set_string(json *, const char *);
json *json_set_integer(json *, double);
json *json_set_real(json *, double);
json *json_set_boolean(json *, int);
json *json_set_null(json *);
json *json_push_front(json *, json *);
json *json_push_back(json *, json *);
json *json_push_before(json *, json *);
json *json_push_after(json *, json *);
json *json_push_at(json *, json *, size_t);
json *json_pop(json *);
json *json_pop_front(json *);
json *json_pop_back(json *);
json *json_pop_at(json *, size_t);
json *json_delete(json *);
void json_free(json *);
void json_merge(json *, json *);
// ============================================================================
// Buffer
// ============================================================================
enum json_encode json_get_encode(void);
void json_set_encode(enum json_encode);
char *json_encode(const json *);
char *json_indent(const json *, int);
int json_write(const json *, FILE *, int);
int json_write_line(const json *, FILE *);
int json_write_file(const json *, const char *, int);
int json_print(const json *);
char *json_path(const json *);
char *json_quote(const char *);
// ============================================================================
// Map
// ============================================================================
json_map *json_map_create(size_t);
json *json_map_update(json_map *, const char *, json *);
json *json_map_insert(json_map *, const char *, json *);
json *json_map_upsert(json_map *, const char *, json *);
json *json_map_delete(json_map *, const char *);
json *json_map_search(const json_map *, const char *);
json *json_map_walk(const json_map *, json_map_walk_callback, void *);
size_t json_map_size(const json_map *);
void json_map_destroy(json_map *, void (*)(json *));
// ============================================================================
// Pointer
// ============================================================================
json *json_pointer(const json *, const char *);
// ============================================================================
// Sort
// ============================================================================
// Predefined sort callbacks
int JSON_SORT_BY_KEY_ASC(const json *, const json *);
int JSON_SORT_BY_KEY_DESC(const json *, const json *);
int JSON_SORT_BY_VALUE_ASC(const json *, const json *);
int JSON_SORT_BY_VALUE_DESC(const json *, const json *);
// End predefined sort callbacks
void json_sort(json *, json_sort_callback);
void json_reverse(json *);
// ============================================================================
// Query
// ============================================================================
int json_is(const json *, const char *);
int json_is_unique(const json *);

// ============================================================================
// Macros 
// ============================================================================
#define json_int(node) ((int)json_number(node))
#define json_uint(node) ((unsigned int)json_number(node))
#define json_long(node) ((long)json_number(node))
#define json_ulong(node) ((unsigned long)json_number(node))
#define json_llong(node) ((long long)json_number(node))
#define json_ullong(node) ((unsigned long long)json_number(node))
#define json_size_t(node) ((size_t)json_number(node))
#define json_float(node) ((float)json_number(node))
#define json_double(node) json_number(node)

#define json_new_number(name, value) _Generic((value),  \
    long double: json_new_real,                         \
    double: json_new_real,                              \
    float: json_new_real,                               \
    default: json_new_integer)(name, (double)(value))

#define json_set_number(node, value) _Generic((value),  \
    long double: json_set_real,                         \
    double: json_set_real,                              \
    float: json_set_real,                               \
    default: json_set_integer)(node, (double)(value))

#endif

