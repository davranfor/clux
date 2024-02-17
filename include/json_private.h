/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PRIVATE_H
#define JSON_PRIVATE_H

#include "json.h"

struct json
{
    json *parent, *head, *prev, *next, *tail;
    char *name; union { char *string; double number; } value;
    enum json_type type;
    unsigned size;
};

#endif

