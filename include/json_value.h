/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/**
 * BE CAREFUL!
 * json_value() returns a struct without allocating memory
 */

#ifndef JSON_VALUE_H
#define JSON_VALUE_H

#include "json_header.h"

#define JSON_STR_SIZE 48

#define json_str(node) (json_value(node).as_string)
#define json_num(node) (json_value(node).as_number)

struct json_value
{
    char str[JSON_STR_SIZE];
    const char *as_string;
    double as_number;
};

struct json_value json_value(const json *);

#endif

