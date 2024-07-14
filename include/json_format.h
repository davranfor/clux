/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_FORMAT_H
#define JSON_FORMAT_H

#include "json_header.h"

#define JSON_AUTO_DECIMALS (-1)

#define json_format(node, decimals) (json_format_number(node, decimals).ptr)
#define json_str(node) (json_to_string(node).ptr)

typedef struct { char str[56]; const char *ptr; } json_format_buffer;

json_format_buffer json_format_number(const json *, int);
json_format_buffer json_to_string(const json *);
double json_to_number(const json *);

#endif

