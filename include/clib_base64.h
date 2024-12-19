/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_BASE64_H
#define CLIB_BASE64_H

char *base64_encode(const unsigned char *, size_t, size_t *);
unsigned char *base64_decode(const char *, size_t, size_t *);

#endif

