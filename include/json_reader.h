/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_READER_H
#define JSON_READER_H

#include "json_header.h"

#define json_str(node) (json_to_string(node).ptr)
#define json_int(node) ((int)json_number(node))
#define json_uint(node) ((unsigned int)json_number(node))
#define json_long(node) ((long)json_number(node))
#define json_ulong(node) ((unsigned long)json_number(node))
#define json_llong(node) ((long long)json_number(node))
#define json_ullong(node) ((unsigned long long)json_number(node))
#define json_size_t(node) ((size_t)json_number(node))
#define json_float(node) ((float)json_number(node))
#define json_double(node) json_number(node)

typedef struct { char str[56]; const char *ptr; } json_converter;
typedef int (*json_walk_callback)(const json *, int, void *);

enum json_type json_type(const json *);
const char *json_type_name(const json *);
const char *json_key(const json *);
const char *json_name(const json *);
const char *json_text(const json *);
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
json *json_object_head(const json *);
json *json_object_tail(const json *);
json *json_array_head(const json *);
json *json_array_tail(const json *);
json *json_at(const json *, size_t);
json *json_find(const json *, const char *);
json *json_find_prev(const json *, const char *);
json *json_find_next(const json *, const char *);
json *json_locate(const json *, const json *);
json *json_locate_prev(const json *, const json *);
json *json_locate_next(const json *, const json *);
json_converter json_convert(const json *, int);
json_converter json_to_string(const json *);
size_t json_length(const json *);
size_t json_size(const json *);
size_t json_offset(const json *);
size_t json_height(const json *);
size_t json_depth(const json *);
int json_compare(const json *, const json *);
int json_equal(const json *, const json *);
int json_walk(const json *, json_walk_callback, void *);

#endif

