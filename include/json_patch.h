/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PATCH_H
#define JSON_PATCH_H

#include "json_header.h"

int json_patch(json *, json *);
int json_unpatch(json *, json *, int);

#endif

