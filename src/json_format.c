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
#include "json_reader.h"
#include "json_format.h"

/**
 * Returns a json_number converted to string
 * Pass JSON_AUTO_DECIMALS as decimals to print using %g 
 */
json_format_buffer json_format_number(const json *node, int decimals)
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
    if (decimals >= 0)
    {
        snprintf(buffer.str, sizeof buffer.str, "%.*lf", decimals, number);
    }
    else
    {
        snprintf(buffer.str, sizeof buffer.str, "%g", number);
    }
    buffer.ptr = buffer.str;
    return buffer;
}

/* Returns a node converted to string */
#define FORMAT(fmt, value) snprintf(buffer.str, sizeof buffer.str, fmt, value)
json_format_buffer json_to_string(const json *node)
{
    json_format_buffer buffer = {.str = ""};

    buffer.ptr = buffer.str;
    if (node != NULL)
    {
        switch (node->type)
        {
            case JSON_STRING:
                buffer.ptr = node->value.string;
                break;
            case JSON_INTEGER:
                FORMAT("%.0f", node->value.number);
                break;
            case JSON_REAL:
                node->value.number != trunc(node->value.number)
                    ? FORMAT("%g", node->value.number)
                    : FORMAT("%.1f", node->value.number);
                break;
            case JSON_BOOLEAN:
                buffer.ptr = node->value.number != 0 ? "true" : "false";
                break;
            default:
                buffer.ptr = json_type_name(node);
                break;
        }
    }
    return buffer;
}

/* Returns a node converted to number */
double json_to_number(const json *node)
{
    if (node == NULL)
    {
        return 0.0;
    }
    if (node->type != JSON_STRING)
    {
        return node->value.number;
    }
    return strtod(node->value.string, NULL);
}

