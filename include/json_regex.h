/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_REGEX_H
#define JSON_REGEX_H

#include "json.h"

int json_regex(const json *, const char *);
int json_match(const json *, const char *);

#endif

