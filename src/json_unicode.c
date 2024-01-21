/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_unicode.h"

/* Returns the number of chars of a sequence or 0 on fail */
size_t special_chars(const char *str)
{
    switch (str[0])
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
        case 'u' :
            return
            (
                is_xdigit(str[1]) &&
                is_xdigit(str[2]) &&
                is_xdigit(str[3]) &&
                is_xdigit(str[4])
            ) ? 5 : 0;
        default:
            return 0;
    }   
}

/**
 * Converts unicode escape sequence to multibyte sequence
 * Returns the length of the multibyte in bytes
 */
static size_t decode_ues(const char *str, char *buf)
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

/* Writes sequence to string */
size_t decode_special_chars(const char *str, char *buf, size_t *size)
{
    switch (*str)
    {
        default : *buf = *str; *size= 1; return 1;
        case 'b': *buf = '\b'; *size= 1; return 1;
        case 'f': *buf = '\f'; *size= 1; return 1;
        case 'n': *buf = '\n'; *size= 1; return 1;
        case 'r': *buf = '\r'; *size= 1; return 1;
        case 't': *buf = '\t'; *size= 1; return 1;
        case 'u': *size = 5; return decode_ues(str + 1, buf);
    }
}

/* Converts escape to char  */
char encode_esc(const char *str)
{
    switch (*str)
    {
        case '\b': return 'b';
        case '\f': return 'f';
        case '\n': return 'n';
        case '\r': return 'r';
        case '\t': return 't';
        case '"' : return '"';
        case '\\': return '\\';
        default  : return '\0';
    }
}

/**
 * Converts multibyte sequence to unicode escape sequence
 * Returns the length of the multibyte in bytes
 */
size_t encode_ues(const char *str, char *buf)
{
    size_t length = 1;
    int ues = str[0];

    if ((str[0] & 0x80) == 0)
    {
        // noop
    }
    else if ((str[0] & 0xe0) == 0xc0)
    {
        if (str[1] != '\0')
        {
            ues = ((str[0] & 0x1f) << 6)
                | ((str[1] & 0x3f) << 0);
            length = 2;
        }
    }
    else if ((str[0] & 0xf0) == 0xe0)
    {
        if ((str[1] != '\0') && (str[2] != '\0'))
        {
            ues = ((str[0] & 0x0f) << 12)
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
            ues = ((str[0] & 0x07) << 18)
                | ((str[1] & 0x3f) << 12)
                | ((str[2] & 0x3f) << 6)
                | ((str[3] & 0x3f) << 0);
            */
            ues = 0xfffd; // Replacement character �
            length = 4;
        }
    }
    snprintf(buf, sizeof("\\u0123"), "\\u%04x", ues);
    return length;
}

/* Reverse chars of a string given a length */ 
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

