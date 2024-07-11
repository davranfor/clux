/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "json_header.h"

typedef struct {int line, column;} json_error;

json *json_parse(const char *, json_error *);
json *json_parse_file(const char *, json_error *);
void json_print_error(const json_error *);

#endif

