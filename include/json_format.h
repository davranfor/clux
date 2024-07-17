/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/**
 * BE CAREFUL!
 * These functions return a struct without allocating memory
 */

#ifndef JSON_FORMAT_H
#define JSON_FORMAT_H

#include "json_header.h"

#define JSON_STR_SIZE 48

#define JSON_FMT_AUTO (-1) // Format infered from the number
#define JSON_FMT_HEX  (-2) // Hexadecimal notation
#define JSON_FMT_SCI  (-3) // Scientific notation

#define json_fmt(node, decimals) (json_format((node), (decimals)).str)
#define json_str(node) (json_value(node).as_string)
#define json_num(node) (json_value(node).as_number)

struct json_format
{
    char str[JSON_STR_SIZE];
};

struct json_value
{
    char str[JSON_STR_SIZE];
    const char *as_string;
    double as_number;
};

struct json_format json_format(const json *, int);
struct json_value json_value(const json *);

#endif

