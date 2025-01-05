/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_CHECK_H
#define CLIB_CHECK_H

/* return 0 if 'expr' fails */
#define CHECK(expr) do { if (!(expr)) return 0; } while (0)

#endif

