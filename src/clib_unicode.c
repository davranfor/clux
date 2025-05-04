/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clib_unicode.h"

/* Returns 1 on escape code, 0 otherwise */
int is_esc(const char *str)
{
    switch (*str)
    {
        case '\\':
        case '/' :
        case '"' :
        case 'b' :
        case 'f' :
        case 'n' :
        case 'r' :
        case 't' :
            return 1;
        default  :
            return 0;
    }
}

/* Decodes escape code and return its value */
char decode_esc(const char *str)
{
    switch (*str)
    {
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        default : return *str;
    }
}

/* Converts escape to char */
char encode_esc(const char *str)
{
    switch (*str)
    {
        case '\b': return 'b';
        case '\f': return 'f';
        case '\n': return 'n';
        case '\r': return 'r';
        case '\t': return 't';
        case '\"': return '"';
        case '\\': return '\\';
        default  : return '\0';
    }
}

/* Returns 1 on unicode escape sequence, 0 otherwise */
int is_hex(const char *str)
{
    return (('u') == str[0])
        && is_xdigit(str[1])
        && is_xdigit(str[2])
        && is_xdigit(str[3])
        && is_xdigit(str[4]);
}

/**
 * Converts unicode escape sequence to multibyte sequence
 * Returns the length of the multibyte in bytes
 */
size_t decode_hex(const char *str, char *buf)
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
 * Converts multibyte sequence to unicode escape sequence
 * Returns the length of the multibyte in bytes
 */
size_t encode_hex(const char *str, char *buf)
{
    size_t length = 1;
    int hex = str[0];

    if ((str[0] & 0x80) == 0)
    {
        // noop
    }
    else if ((str[0] & 0xe0) == 0xc0)
    {
        if (str[1] != '\0')
        {
            hex = ((str[0] & 0x1f) << 6)
                | ((str[1] & 0x3f) << 0);
            length = 2;
        }
    }
    else if ((str[0] & 0xf0) == 0xe0)
    {
        if ((str[1] != '\0') && (str[2] != '\0'))
        {
            hex = ((str[0] & 0x0f) << 12)
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
            UES are restricted to 3 bytes and can not be represented as
            hex = ((str[0] & 0x07) << 18)
                | ((str[1] & 0x3f) << 12)
                | ((str[2] & 0x3f) << 6)
                | ((str[3] & 0x3f) << 0);
            */
            hex = 0xfffd; // Replacement character �
            length = 4;
        }
    }
    snprintf(buf, sizeof("\\u0123"), "\\u%04x", hex);
    return length;
}

int hex_to_dec(int c)
{
    if ((c >= '0') && (c <= '9'))
    {
        return c - '0';
    }
    if ((c >= 'A') && (c <= 'F'))
    {
        return c - 'A' + 10;
    }
    if ((c >= 'a') && (c <= 'f'))
    {
        return c - 'a' + 10;
    }
    return -1;
}

