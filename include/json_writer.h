/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_WRITER_H
#define JSON_WRITER_H

#include "json_header.h"

#define json_new_number(number) _Generic((number),  \
    long double: json_new_real,                     \
    double: json_new_real,                          \
    float: json_new_real,                           \
    default: json_new_integer)((double)(number))

#define JSON_PUSH(_1, _2, _3, _4, NAME, ...) NAME
#define json_push(...) \
    JSON_PUSH(__VA_ARGS__, json_object_push, json_push_child, )(__VA_ARGS__)

#define json_push_front(parent, ...)    \
    json_push(parent, JSON_HEAD, __VA_ARGS__)
#define json_push_back(parent, ...) \
    json_push(parent, JSON_TAIL, __VA_ARGS__)

#define json_pop(parent, what) _Generic((what), \
    char *: json_object_pop,                    \
    void *: json_object_pop,                    \
    default: json_pop_child)((parent), (what))

#define json_pop_front(parent) \
    json_pop_child(parent, JSON_HEAD)
#define json_pop_back(parent) \
    json_pop_child(parent, JSON_TAIL)

#define json_move_from_to(parent, a, b) \
    json_move_child(parent, a, parent, b)

#define JSON_MOVE(_1, _2, _3, _4, NAME, ...) NAME
#define json_move(...) \
    JSON_MOVE(__VA_ARGS__, json_move_child, json_move_from_to, )(__VA_ARGS__)

#define json_swap_from_to(parent, a, b) \
    json_swap_child(parent, a, parent, b)

#define JSON_SWAP(_1, _2, _3, _4, NAME, ...) NAME
#define json_swap(...) \
    JSON_SWAP(__VA_ARGS__, json_swap_child, json_swap_from_to, )(__VA_ARGS__)

#define json_delete_at(parent, what) _Generic((what),   \
    char *: json_object_delete,                         \
    void *: json_object_delete,                         \
    default: json_delete_child)((parent), (what))

#define JSON_DELETE(_1, _2, NAME, ...) NAME
#define json_delete(...) \
    JSON_DELETE(__VA_ARGS__, json_delete_at, json_delete_root, )(__VA_ARGS__)

#define json_delete_front(parent) \
    json_delete_child(parent, JSON_HEAD)
#define json_delete_back(parent) \
    json_delete_child(parent, JSON_TAIL)

json_t *json_new_object(void);
json_t *json_new_array(void);
json_t *json_new_string(const char *);
json_t *json_new_format(const char *, ...)
    __attribute__ ((format (printf, 1, 2)));
json_t *json_new_integer(double);
json_t *json_new_real(double);
json_t *json_new_boolean(int);
json_t *json_new_null(void);
json_t *json_object_push(json_t *, size_t, const char *, json_t *);
json_t *json_array_push(json_t *, size_t, json_t *);
json_t *json_push_child(json_t *, size_t, json_t *);
json_t *json_object_pop(json_t *, const char *);
json_t *json_array_pop(json_t *, size_t);
json_t *json_pop_child(json_t *, size_t);
json_t *json_move_child(json_t *, size_t, json_t *, size_t);
json_t *json_swap_child(json_t *, size_t, json_t *, size_t);
int json_object_delete(json_t *, const char *);
int json_array_delete(json_t *, size_t);
int json_delete_child(json_t *, size_t);
int json_delete_root(json_t *);
void json_free(json_t *);

#endif

