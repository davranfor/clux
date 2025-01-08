/*!
 *  \brief     clux - json_t and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_BUFFER_H
#define JSON_BUFFER_H

#include <stdio.h>
#include "clib_buffer.h"
#include "json_header.h"

enum json_encode {JSON_UTF8, JSON_ASCII};

enum json_encode json_get_encode(void);
void json_set_encode(enum json_encode);
char *json_encode(const json_t *, size_t);
char *json_encode_max(const json_t *, size_t, size_t);
char *json_buffer_encode(buffer_t *, const json_t *, size_t);
char *json_buffer_encode_max(buffer_t *, const json_t *, size_t, size_t);
char *json_stringify(const json_t *);
int json_write(const json_t *, FILE *, size_t);
int json_write_line(const json_t *, FILE *);
int json_write_file(const json_t *, const char *, size_t);
int json_print(const json_t *);
char *json_quote(const char *);
char *json_buffer_quote(buffer_t *, const char *);
char *json_convert(double, enum json_type);
char *json_buffer_convert(buffer_t *, double, enum json_type);

#endif

