/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_REGEX_H
#define JSON_REGEX_H

#include "json_header.h"

int json_regex(const json_t *, const char *);
int json_match(const json_t *, const char *);

#endif

