/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "clib_test.h"
#include "json_private.h"
#include "json_regex.h"

int json_regex(const json *node, const char *str)
{
    if ((node != NULL) && (node->type == JSON_STRING) && (str != NULL))
    {
        return test_regex(node->value.string, str);
    }
    return 0;
}

int json_match(const json *node, const char *str)
{
    if ((node != NULL) && (node->type == JSON_STRING) && (str != NULL))
    {
        return test_match(node->value.string, str);
    }
    return 0;
}

