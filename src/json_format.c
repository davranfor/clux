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

/**
 * Returns a json_number converted to string
 * Pass JSON_AUTO_DECIMALS as decimals to print using %g 
 */
json_format_buffer json_format(const json *node, int decimals)
{
    json_format_buffer buffer;
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
    if ((decimals >= 0) && (decimals <= DBL_DECIMAL_DIG))
    {
        snprintf(buffer.str, sizeof buffer.str, "%.*f", decimals, number);
    }
    else
    {
        snprintf(buffer.str, sizeof buffer.str, "%g", number);
    }
    return buffer;
}

/* Returns a node converted to string */
#define FORMAT(fmt, ...) snprintf(buffer.str, sizeof buffer.str, fmt, __VA_ARGS__)
json_value_buffer json_value(const json *node)
{
    json_value_buffer buffer = {.str = ""};

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
                FORMAT("%.0f", node->value.number);
                buffer.as_number = node->value.number;
                break;
            case JSON_REAL:
                node->value.number != trunc(node->value.number)
                    ? FORMAT("%.*g", DBL_DECIMAL_DIG, node->value.number)
                    : FORMAT("%.1f", node->value.number);
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

