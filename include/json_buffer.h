/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_BUFFER_H
#define JSON_BUFFER_H

#include <stdio.h>
#include "json_header.h"

enum json_encode
{
    JSON_UTF8,
    JSON_ASCII,
};

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

#endif

