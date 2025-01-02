/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef CLIB_LOCALE_H
#define CLIB_LOCALE_H

#if __APPLE__
#include <xlocale.h>
#elif __unix__
#include <locale.h>
#else
#error "Unsupported system"
#endif

locale_t locale_numeric(void);

#endif

