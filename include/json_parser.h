/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "json_header.h"

typedef struct {int line, column;} json_error_t;

void json_parser_set_max_depth(unsigned short);
unsigned short json_parser_get_max_depth(void);
json_t *json_parse(const char *, json_error_t *);
json_t *json_parse_file(const char *, json_error_t *);
void json_print_error(const json_error_t *);

#endif

