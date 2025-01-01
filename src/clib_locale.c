/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include "clib_locale.h"

/* Make strtod independent of locales */
static locale_t c_numeric;

__attribute__((constructor))
static void create_locale_numeric(void)
{
    c_numeric = newlocale(LC_NUMERIC_MASK, "C", NULL);
}

__attribute__((destructor))
static void free_locale_numeric(void)
{
    freelocale(c_numeric);
}

locale_t l_numeric(void)
{
    return c_numeric;
}

