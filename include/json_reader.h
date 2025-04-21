/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_READER_H
#define JSON_READER_H

#include "json_header.h"

#define json_int(node) ((int)json_number(node))
#define json_int8_t(node) ((int8_t)json_number(node))
#define json_int16_t(node) ((int16_t)json_number(node))
#define json_int32_t(node) ((int32_t)json_number(node))
#define json_int64_t(node) ((int64_t)json_number(node))
#define json_uint(node) ((unsigned)json_number(node))
#define json_uint8_t(node) ((uint8_t)json_number(node))
#define json_uint16_t(node) ((uint16_t)json_number(node))
#define json_uint32_t(node) ((uint32_t)json_number(node))
#define json_uint64_t(node) ((uint64_t)json_number(node))
#define json_long(node) ((long)json_number(node))
#define json_ulong(node) ((unsigned long)json_number(node))
#define json_llong(node) ((long long)json_number(node))
#define json_ullong(node) ((unsigned long long)json_number(node))
#define json_size_t(node) ((size_t)json_number(node))
#define json_float(node) ((float)json_number(node))
#define json_double(node) json_number(node)

typedef int (*json_walk_callback)(const json_t *, size_t, void *);

char *json_key(const json_t *);
char *json_name(const json_t *);
char *json_string(const json_t *);
char *json_text(const json_t *);
double json_number(const json_t *);
int json_boolean(const json_t *);
int json_is_iterable(const json_t *);
int json_is_scalar(const json_t *);
int json_is_object(const json_t *);
int json_is_array(const json_t *);
int json_is_string(const json_t *);
int json_is_integer(const json_t *);
int json_is_unsigned(const json_t *);
int json_is_real(const json_t *);
int json_is_number(const json_t *);
int json_is_boolean(const json_t *);
int json_is_true(const json_t *);
int json_is_false(const json_t *);
int json_is_null(const json_t *);
int json_is_root(const json_t *);
int json_is_child(const json_t *);
enum json_type json_type(const json_t *);
unsigned short json_flags(const json_t *);
size_t json_height(const json_t *);
unsigned json_size(const json_t *);
unsigned json_properties(const json_t *);
unsigned json_items(const json_t *);
unsigned json_index(const json_t *, const char *);
const json_t * const *json_child(const json_t *);
json_t *json_head(const json_t *);
json_t *json_tail(const json_t *);
json_t *json_at(const json_t *, size_t);
json_t *json_find(const json_t *, const char *);
json_t *json_locate(const json_t *, const json_t *);
int json_match(const json_t *, const char *);
int json_regex(const json_t *, const char *);
int json_is_unique(const json_t *, const json_t *);
int json_unique_children(const json_t *);
int json_equal(const json_t *, const json_t *);
int json_walk(const json_t *, json_walk_callback, void *);

#endif

