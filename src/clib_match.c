/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clib_date.h"
#include "clib_unicode.h"
#include "clib_match.h"

const char *test_mask(const char *text, const char *mask)
{
    /**
     *  \'  quote text until next quote (inner quotes must be escaped with \\)
     *  \\  next character is a literal (not a function) (required)
     *  \?  next character is a literal (not a function) (optional)
     *  0   is_digit (required)
     *  9   is_digit (optional)
     *  A   is_alpha (required)
     *  a   is_alpha (optional)
     *  W   is_alnum (required)
     *  w   is_alnum (optional)
     *  X   is_xdigit (required)
     *  x   is_xdigit (optional)
     *  *   return the string at this position
     */

    int quoted = 0;

    while (*mask != '\0')
    {
        int (*function)(int) = NULL;
        int required = 0;

        if (quoted)
        {
            if (*mask == '\'')
            {
                quoted = 0;
                mask++;
                continue;
            }
            if (*mask == '\\')
            {
                mask++;
            }
            required = 1;
        }
        else switch (*mask)
        {
            case '\'':
                quoted = 1;
                mask++;
                continue;
            case '\\':
                required = 1;
                mask++;
                break;
            case '\?':
                mask++;
                break;
            case '0':
                function = is_digit;
                required = 1;
                break;
            case '9':
                function = is_digit;
                break;
            case 'A':
                function = is_alpha;
                required = 1;
                break;
            case 'a':
                function = is_alpha;
                break;
            case 'W':
                function = is_alnum;
                required = 1;
                break;
            case 'w':
                function = is_alnum;
                break;
            case 'X':
                function = is_xdigit;
                required = 1;
                break;
            case 'x':
                function = is_xdigit;
                break;
            default:
                required = 1;
                break;
            case '*':
                return text;
        }

        int valid = function ? function(*text) : *text == *mask;

        if (valid)
        {
            if (*text == '\0')
            {
                break;
            }
            text++;
        }
        else if (required)
        {
            return NULL;
        }
        if (*mask != '\0')
        {
            mask++;
        }
    }
    return *text == *mask ? text : NULL;
}

static const char *test_date(const char *str)
{
    const char *date = test_mask(str, "0000-00-00*");

    if (date != NULL)
    {
        if (is_date((int)strtol(&str[0], NULL, 10),
                    (int)strtol(&str[5], NULL, 10),
                    (int)strtol(&str[8], NULL, 10)))
        {
            return date;
        }
    }
    return NULL;
}

int test_is_date(const char *str)
{
    return (str = test_date(str)) && (*str == '\0');
}

static int is_time_suffix(const char *str)
{
    return test_mask(str, "+09:00")
        || test_mask(str, "-09:00")
        || test_mask(str, "\\Z");
}

int test_is_time(const char *str)
{
    const char *time = test_mask(str, "00:00:00*");

    if (time != NULL)
    {
        if ((strtol(&str[0], NULL, 10) < 24) &&
            (strtol(&str[3], NULL, 10) < 60) &&
            (strtol(&str[6], NULL, 10) < 60))
        {
            return *time ? is_time_suffix(time) : 1;
        }
    }
    return 0;
}

int test_is_date_time(const char *str)
{
    if ((str = test_date(str)) && (*str == 'T'))
    {
        return test_is_time(str + 1);
    }
    return 0;
}

static const char *test_hostname(const char *str)
{
    // Must start with a digit or alpha character
    if (!is_alnum(*str))
    {
        return NULL;
    }

    int label_length = 0, length = 0;

    while (*str != '\0')
    {
        // Don't allow "--" or "-." or ".-" or ".."
        if (((str[0] == '-') || (str[0] == '.')) &&
            ((str[1] == '-') || (str[1] == '.')))
        {
            return NULL;
        }
        // Each label must be between 1 and 63 characters long
        // The entire hostname (including the delimiting dots
        // but not a trailing dot) has a maximum of 253 chars
        if ((*str == '-') || is_alnum(*str))
        {
            if ((label_length++ == 63) || (length >= 253))
            {
                return NULL;
            }
        }
        else if (*str == '.')
        {
            label_length = 0;
        }
        else
        {
            return NULL;
        }
        length++;
        str++;
    }
    // Can not end with hyphen
    return str[-1] != '-' ? str : NULL;
}

int test_is_hostname(const char *str)
{
    return test_hostname(str) ? 1 : 0;
}

int test_is_email(const char *str)
{
    // The local part can not start with space, dot or at symbol
    if ((*str == ' ') || (*str == '.') || (*str == '@'))
    {
        return 0;
    }

    int mbs = 0;

    // Max. 63 UTF8 chars in the local part
    while ((*str != '@') && (*str != '\0'))
    {
        if (is_utf8(*str) && (mbs++ == 63))
        {
            return 0;
        }
        str++;
    }
    // The local part can not end with a dot
    if ((str[0] != '@') || (str[-1] == '.'))
    {
        return 0;
    }
    // The domain part can not end with a dot
    return (str = test_hostname(str + 1)) && (str[-1] != '.');
}

int test_is_ipv4(const char *str)
{
    if (!test_mask(str, "099.099.099.099"))
    {
        return 0;
    }
    for (int byte = 0; byte < 4; byte++)
    {
        char *end;

        if (strtol(str, &end, 10) < 256)
        {
            str = end + 1;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

/**
 * An ipv6 addresses can consist of:
 * - 2-6 ipv6 segments abbreviated with a double colon with or without ipv4
 * - 6 ipv6 segments separated by single colons and required ipv4
 * - 6-8 ipv6 segments abbreviated with a double colon without ipv4
 * - 8 ipv6 segments separated by single colons without ipv4
 */
int test_is_ipv6(const char *str)
{
    const char *mask = "xxxx:*", *end = str, *next;
    int colons = 0, abbrv = 0;

    while ((next = test_mask(end, mask)) && (colons < 7))
    {
        // Double colon may only be used once
        if ((colons++ > 0) && (next == end + 1))
        {
            if (abbrv++ > 0)
            {
                return 0;
            }
        }
        end = next;
    }
    // Can not start with a single colon (except abbrv '::')
    if ((str[0] == ':') && (str[1] != ':'))
    {
        return 0;
    }
    // 6 ipv6 segments separated by single colons and required ipv4
    if ((colons == 5) && (abbrv == 0))
    {
        return test_is_ipv4(end);
    }
    // 6-8 ipv6 segments abbreviated with double colon without ipv4
    if ((colons >= 5) && (abbrv == 1))
    {
        return test_mask(end, "xxxx") ? 1 : 0;
    }
    // 8 ipv6 segments separated by single colons without ipv4
    if ((colons == 7) && (abbrv == 0))
    {
        return test_mask(end, "Xxxx") ? 1 : 0;
    }
    // 2-6 segments abbreviated with a double colon with or without ipv4
    return (abbrv == 1) && (test_mask(end, "xxxx") || test_is_ipv4(end));
}

int test_is_uuid(const char *str)
{
    return test_mask(str, "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX") ? 1 : 0;
}

int test_is_url(const char *str)
{
    const char *url = test_mask(str, "\'http\'\?s://*");

    if (url && *url)
    {
        const char *allow = "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "0123456789"
                            "-._~:/?#[]@!$&'()*+,;%=";

        size_t end = strspn(str, allow);

        // Maximum of 2048 characters
        return (end <= 2048) && (str[end] == '\0');
    }
    return 0;
}

int test_is_identifier(const char *str)
{
    const char *allow = "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "0123456789_";

    return str[0] && !is_digit(str[0]) && !str[strspn(str, allow)];
}

int test_match(const char *text, const char *pattern)
{
    int (*test)(const char *) =
        !strcmp(pattern, "date") ? test_is_date :
        !strcmp(pattern, "time") ? test_is_time :
        !strcmp(pattern, "date-time") ? test_is_date_time :
        !strcmp(pattern, "hostname") ? test_is_hostname :
        !strcmp(pattern, "email") ? test_is_email :
        !strcmp(pattern, "ipv4") ? test_is_ipv4 :
        !strcmp(pattern, "ipv6") ? test_is_ipv6 :
        !strcmp(pattern, "uuid") ? test_is_uuid :
        !strcmp(pattern, "url") ? test_is_url :
        !strcmp(pattern, "identifier") ? test_is_identifier : NULL;

    return test != NULL ? test(text) : 0;
}

