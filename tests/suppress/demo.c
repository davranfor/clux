/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/* suppress cppcheck unused functions messages of the library */

#include <clux/json.h>

int main(void)
{
    (void)json_get_encode;
    (void)json_set_encode;
    (void)json_write_file;
    (void)json_quote;
    (void)json_type_name;
    (void)json_typeof_name;
    (void)json_integer;
    (void)json_prev;
    (void)json_tail;
    (void)json_find_prev;
    (void)json_depth;
    (void)json_swap;
    (void)json_set_name;
    (void)json_set_object;
    (void)json_set_array;
    (void)json_set_format;
    (void)json_set_number;
    (void)json_set_boolean;
    (void)json_set_null;
    return 0;
}

