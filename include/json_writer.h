/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include "json_header.h"

#define json_new_number(value) _Generic((value),                \
    long double: json_new_real,                                 \
    double: json_new_real,                                      \
    float: json_new_real,                                       \
    default: json_new_integer)((double)(value))

#define json_new_named_number(name, value) _Generic((value),    \
    long double: json_new_named_real,                           \
    double: json_new_named_real,                                \
    float: json_new_named_real,                                 \
    default: json_new_named_integer)(name, (double)(value))

#define json_set_number(node, value) _Generic((value),          \
    long double: json_set_real,                                 \
    double: json_set_real,                                      \
    float: json_set_real,                                       \
    default: json_set_integer)(node, (double)(value))

#define json_let_number(parent, name, value) _Generic((value),  \
    long double: json_let_real,                                 \
    double: json_let_real,                                      \
    float: json_let_real,                                       \
    default: json_let_integer)(parent, name, (double)(value))

json *json_new_object(void);
json *json_new_array(void);
json *json_new_format(const char *, ...)
    __attribute__ ((format (printf, 1, 2)));
json *json_new_string(const char *);
json *json_new_integer(double);
json *json_new_real(double);
json *json_new_boolean(int);
json *json_new_null(void);
json *json_new_named_object(const char *);
json *json_new_named_array(const char *);
json *json_new_named_format(const char *, const char *, ...)
    __attribute__ ((format (printf, 2, 3)));
json *json_new_named_string(const char *, const char *);
json *json_new_named_integer(const char *, double);
json *json_new_named_real(const char *, double);
json *json_new_named_boolean(const char *, int);
json *json_new_named_null(const char *);
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
json *json_let_object(json *, const char *);
json *json_let_array(json *, const char *);
json *json_let_format(json *, const char *, const char *, ...)
    __attribute__ ((format (printf, 3, 4)));
json *json_let_string(json *, const char *, const char *);
json *json_let_integer(json *, const char *, double);
json *json_let_real(json *, const char *, double);
json *json_let_boolean(json *, const char *, int);
json *json_let_null(json *, const char *);
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

#endif

