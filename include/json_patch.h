/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PATCH_H
#define JSON_PATCH_H

#include "json_header.h"

int json_patch(json_t *, json_t *);
void json_unpatch(json_t *, json_t *, int);

#endif

