/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/**
Generate and save a new universally unique identifier (UUID) as a json string

Compile and run with:
CFLAGS="-std=c11 -Wpedantic -Wall -Wextra -O2" LDLIBS="-luuid -lclux" make demo && ./demo
*/

#include <uuid/uuid.h>
#include <clux/json.h>

int main(void)
{
    uuid_t uuid;

    uuid_generate(uuid);

    char str[37];

    uuid_unparse(uuid, str);

    json *node = json_new_string("UUID", str);

    json_print(node);
    json_free(node);
    return 0;
}

