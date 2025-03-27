/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef SCHEMA_H
#define SCHEMA_H


void schema_load(void);
void schema_reload(void);
int schema_push(const char *);

#endif

