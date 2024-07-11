/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "clib_format.h"
#include "json_regex.h"

int json_regex(const json *node, const char *str)
{
    if (json_is_string(node) && (str != NULL))
    {
        return test_regex(json_string(node), str);
    }
    return 0;
}

int json_match(const json *node, const char *str)
{
    if (json_is_string(node) && (str != NULL))
    {
        return test_match(json_string(node), str);
    }
    return 0;
}

