/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_FORMAT_H
#define JSON_FORMAT_H

#include "json_header.h"

#define JSON_AUTO_DECIMALS (-1)

#define json_fmt(node, decimals) (json_format((node), (decimals)).str)
#define json_str(node) (json_value(node).as_string)
#define json_num(node) (json_value(node).as_number)

typedef struct
{
    char str[64];
} json_format_buffer;

typedef struct
{
    char str[48];
    const char *as_string;
    double as_number;
} json_value_buffer;

json_format_buffer json_format(const json *, int);
json_value_buffer json_value(const json *);

#endif

