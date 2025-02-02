/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef REST_H
#define REST_H

char *rest_get(const char *);
char *rest_post(const char *, const char *);
char *rest_put(const char *, const char *);
char *rest_patch(const char *, const char *);
char *rest_delete(const char *);

#endif

