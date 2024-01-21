/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_UNICODE_H
#define JSON_UNICODE_H

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

static inline int is_space(int c)
{
    return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t');
}

size_t special_chars(const char *);
size_t decode_special_chars(const char *, char *, size_t *);
char encode_esc(const char *);
size_t encode_ues(const char *, char *);

void string_reverse(char *, size_t);

#endif

