/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_UNICODE_H
#define CLIB_UNICODE_H

#include <stddef.h>

#define is_utf8(c) (((c) & 0xc0) != 0x80)

static inline int is_cntrl(int c)
{
    return (c >= 0) && (c <= 0x1f);
}

static inline int is_ascii(int c)
{
    return (c >= 0) && (c <= 0x7f);
}

static inline int is_digit(int c)
{
    return (c >= '0') && (c <= '9');
}

static inline int is_xdigit(int c)
{
    return ((c >= '0') && (c <= '9'))
        || ((c >= 'A') && (c <= 'F'))
        || ((c >= 'a') && (c <= 'f'));
}

static inline int is_alpha(int c)
{
    return ((c >= 'A') && (c <= 'Z'))
        || ((c >= 'a') && (c <= 'z'));
}

static inline int is_alnum(int c)
{
    return ((c >= '0') && (c <= '9'))
        || ((c >= 'A') && (c <= 'Z'))
        || ((c >= 'a') && (c <= 'z'));
}

static inline int is_print(int c)
{
    return (c >= 0x20) && (c <= 0x7e);
}

static inline int is_space(int c)
{
    return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t');
}

int is_esc(const char *);
char decode_esc(const char *);
char encode_esc(const char *);
int is_hex(const char *);
size_t decode_hex(const char *, char *);
size_t encode_hex(const char *, char *);
int hex_to_dec(int);

#endif

