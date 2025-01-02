/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef JSON_PRIVATE_H
#define JSON_PRIVATE_H

struct json
{
    char *key;
    union { struct json **child; char *string; double number; };
    unsigned size;          // Size of an iterable (object/array)
    unsigned short flags;   // Available for user
    unsigned char packed;   // 0 = Root node | 1 = Packed node
    unsigned char type;     // json_type_t compressed (1 byte)
};

#endif

