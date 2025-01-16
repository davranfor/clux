/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include "json_header.h"

/* Macros deducing the type of 'number' */
#define json_new_number(number) _Generic((number),  \
    long double: json_new_real,                     \
    double: json_new_real,                          \
    float: json_new_real,                           \
    default: json_new_integer)((double)(number))

#define json_set_number(node, number) _Generic((number),    \
    long double: json_set_real,                             \
    double: json_set_real,                                  \
    float: json_set_real,                                   \
    default: json_set_integer)(node, (double)(number))

json_t *json_new_object(void);
json_t *json_new_array(void);
json_t *json_new_string(const char *);
json_t *json_new_format(const char *, ...) __attribute__ ((format (printf, 1, 2)));
json_t *json_new_integer(double);
json_t *json_new_real(double);
json_t *json_new_boolean(int);
json_t *json_new_null(void);
json_t *json_set_key(json_t *, const char *);
json_t *json_unset_key(json_t *);
json_t *json_set_object(json_t *);
json_t *json_set_array(json_t *);
json_t *json_set_string(json_t *, const char *);
json_t *json_set_format(json_t *, const char *, ...) __attribute__ ((format (printf, 2, 3)));
json_t *json_set_integer(json_t *, double);
json_t *json_set_real(json_t *, double);
json_t *json_set_boolean(json_t *, int);
json_t *json_set_null(json_t *);
json_t *json_set_flags(json_t *, unsigned short);
json_t *json_object_push(json_t *, size_t, const char *, json_t *);
json_t *json_array_push(json_t *, size_t, json_t *);
json_t *json_push_at(json_t *, size_t, json_t *);
json_t *json_object_pop(json_t *, const char *);
json_t *json_array_pop(json_t *, size_t);
json_t *json_pop_at(json_t *, size_t);
json_t *json_move(json_t *, size_t, json_t *, size_t);
json_t *json_swap(json_t *, size_t, json_t *, size_t);
int json_object_delete(json_t *, const char *);
int json_array_delete(json_t *, size_t);
int json_delete_at(json_t *, size_t);
int json_delete_children(json_t *);
int json_delete(json_t *);
void json_free(void *);

#endif

