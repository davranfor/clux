/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_private.h"
#include "json_value.h"

#define format(arg, fmt, ...) snprintf(arg.str, JSON_STR_SIZE, fmt, __VA_ARGS__)

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
            {
                buffer.as_string = node->value.string;
                buffer.as_number = strtod(node->value.string, NULL);
                break;
            }
            case JSON_INTEGER:
            {
                format(buffer, "%.0f", node->value.number);
                buffer.as_number = node->value.number;
                break;
            }
            case JSON_REAL:
            {
                format(buffer, "%.*g", JSON_DECIMAL_DIG, node->value.number);
                size_t end = strspn(buffer.str, "-0123456789");

                if (buffer.str[end] == '\0')
                {
                    snprintf(buffer.str + end, JSON_STR_SIZE - end, ".0");
                }
                buffer.as_number = node->value.number;
                break;
            }
            case JSON_BOOLEAN:
            {
                buffer.as_string = node->value.number != 0 ? "true" : "false";
                buffer.as_number = node->value.number;
                break;
            }
            case JSON_NULL:
            {
                buffer.as_string = "null";
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return buffer;
}

