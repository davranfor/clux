/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_unicode.h"

/* Check whether a character is an escape code */ 
int is_esc(const char *str)
{
    char c = *str;

    return (c == '\\') || (c == '/') || (c == '"')
        || (c == 'b')  || (c == 'f') || (c == 'n') || (c == 'r') || (c == 't');
}

/* Check whether a character is an "Universal character name" */
int is_ucn(const char *str)
{
    return ((*str++) == 'u')
        && is_xdigit(*str++)
        && is_xdigit(*str++)
        && is_xdigit(*str++)
        && is_xdigit(*str);
}

/* Converts char to escape sequence */
int to_esc(const char *str, char *buf)
{
    switch (*str)
    {
        case 'b': *buf = '\b'; return 1;
        case 'f': *buf = '\f'; return 1;
        case 'n': *buf = '\n'; return 1;
        case 'r': *buf = '\r'; return 1;
        case 't': *buf = '\t'; return 1;
        default : *buf = *str; return 1;
    }
}

/**
 * Converts UCN to multibyte sequence
 * Returns the length of the multibyte in bytes
 */
int to_mbs(const char *str, char *buf)
{
    char hex[5] = "";

    memcpy(hex, str, 4);

    unsigned codepoint = (unsigned)strtoul(hex, NULL, 16);

    if (codepoint <= 0x7f)
    {
        buf[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7ff)
    {
        buf[0] = (char)(0xc0 | ((codepoint >> 6) & 0x1f));
        buf[1] = (char)(0x80 | ((codepoint >> 0) & 0x3f));
        return 2;
    }
    // if (codepoint <= 0xffff)
    {
        buf[0] = (char)(0xe0 | ((codepoint >> 12) & 0x0f));
        buf[1] = (char)(0x80 | ((codepoint >>  6) & 0x3f));
        buf[2] = (char)(0x80 | ((codepoint >>  0) & 0x3f));
        return 3;
    }
}

/**
 * Converts multibyte sequence to UCN
 * Returns the length of the multibyte in bytes
 */
int to_ucn(const char *str, char *buf)
{
    int ucn = str[0];
    int length = 1;

    if ((str[0] & 0x80) == 0)
    {
        // noop
    }
    else if ((str[0] & 0xe0) == 0xc0)
    {
        if (str[1] != '\0')
        {
            ucn = ((str[0] & 0x1f) << 6)
                | ((str[1] & 0x3f) << 0);
            length = 2;
        }
    }
    else if ((str[0] & 0xf0) == 0xe0)
    {
        if ((str[1] != '\0') && (str[2] != '\0'))
        {
            ucn = ((str[0] & 0x0f) << 12)
                | ((str[1] & 0x3f) << 6)
                | ((str[2] & 0x3f) << 0);
            length = 3;
        }
    }
    else if ((str[0] & 0xf8) == 0xf0)
    {
        if ((str[1] != '\0') && (str[2] != '\0') && (str[3] != '\0'))
        {
            /*
            JSON UCNs are restricted to 3 bytes so it can not be represented as
            ucn = ((str[0] & 0x07) << 18)
                | ((str[1] & 0x3f) << 12)
                | ((str[2] & 0x3f) << 6)
                | ((str[3] & 0x3f) << 0);
            */
            ucn = 0xfffd; // Replacement character �
            length = 4;
        }
    }
    snprintf(buf, sizeof("\\u0123"), "\\u%04x", ucn);
    return length;
}

/* Reverse bytes in a string given a length */ 
void string_reverse(char *str, size_t length)
{
    if (length <= 1)
    {
        return;
    }
    for (size_t i = 0, j = length - 1; i < j; i++, j--)
    {
        char c = str[i];

        str[i] = str[j];
        str[j] = c;
    }
}

