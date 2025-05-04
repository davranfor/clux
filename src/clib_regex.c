/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stddef.h>
#include <regex.h>
#include "clib_regex.h"

int test_regex(const char *text, const char *pattern)
{
    regex_t regex;
    int valid = 0;

    if (!regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB))
    {
        valid = regexec(&regex, text, 0, NULL, 0) == 0;
    }
    regfree(&regex);
    return valid;
}

