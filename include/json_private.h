/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PRIVATE_H
#define JSON_PRIVATE_H

#include "json.h"

union json_value
{
    char *string;
    double number;
};

struct json
{
    json *parent, *child, *prev, *next, *tail;
    char *name;
    union json_value value;
    enum json_type type;
};

#endif

