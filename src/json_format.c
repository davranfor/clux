/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "json_private.h"
#include "json_format.h"

#ifndef DBL_DECIMAL_DIG
#define DBL_DECIMAL_DIG 17
#endif

#define MAX_DECIMALS DBL_DECIMAL_DIG

#define format(arg, fmt, ...) snprintf(arg.str, JSON_STR_SIZE, fmt, __VA_ARGS__)

typedef unsigned long long ullong;

/* Returns a json_number converted to string */
struct json_format json_format(const json *node, int decimals)
{
    struct json_format buffer;
    double number = 0;

    if (node == NULL)
    {
        number = 0.0;
    }
    else if (node->type == JSON_STRING)
    {
        number = strtod(node->value.string, NULL);
    }
    else
    {
        number = node->value.number;
    }
    if ((decimals >= 0) && (decimals <= MAX_DECIMALS))
    {
        format(buffer, "%.*f", decimals, number);
    }
    else if (decimals == JSON_FMT_HEX)
    {
        format(buffer, "0x%llx", (ullong)number);
    }
    else if (decimals == JSON_FMT_SCI)
    {
        format(buffer, "%e", number);
    }
    else // JSON_FMT_AUTO
    {
        format(buffer, "%g", number);
    }
    return buffer;
}

/* Returns a node converted to string and number */
struct json_value json_value(const json *node)
{
    struct json_value buffer = {.str = ""};

    buffer.as_string = buffer.str;
    if (node != NULL)
    {
        switch (node->type)
        {
            case JSON_STRING:
                buffer.as_string = node->value.string;
                buffer.as_number = strtod(node->value.string, NULL);
                break;
            case JSON_INTEGER:
                format(buffer, "%.0f", node->value.number);
                buffer.as_number = node->value.number;
                break;
            case JSON_REAL:
                node->value.number != trunc(node->value.number)
                    ? format(buffer, "%.*g", MAX_DECIMALS, node->value.number)
                    : format(buffer, "%.1f", node->value.number);
                buffer.as_number = node->value.number;
                break;
            case JSON_BOOLEAN:
                buffer.as_string = node->value.number != 0 ? "true" : "false";
                buffer.as_number = node->value.number;
                break;
            case JSON_NULL:
                buffer.as_string = "null";
                break;
            default:
                break;
        }
    }
    return buffer;
}

