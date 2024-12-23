/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include <stdint.h>
#include "clib_base64.h"

char *base64_encode(const unsigned char *data,
    size_t input_length, size_t *output_length)
{
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/";
    static const size_t mod[] = {0, 2, 1};

    *output_length = 4 * ((input_length + 2) / 3);

    char *encoded = malloc(*output_length + 1);

    if (encoded == NULL)
    {
        return NULL;
    }
    for (size_t i = 0, j = 0; i < input_length;)
    {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded[j++] = table[(triple >> 3 * 6) & 0x3F];
        encoded[j++] = table[(triple >> 2 * 6) & 0x3F];
        encoded[j++] = table[(triple >> 1 * 6) & 0x3F];
        encoded[j++] = table[(triple >> 0 * 6) & 0x3F];
    }
    for (size_t i = 0; i < mod[input_length % 3]; i++)
    {
        encoded[*output_length - 1 - i] = '=';
    }
    encoded[*output_length] = '\0';
    return encoded;
}

unsigned char *base64_decode(const char *data,
    size_t input_length, size_t *output_length)
{
    static const uint8_t table[256] =
    {
        ['A'] =  0, ['B'] =  1, ['C'] =  2, ['D'] =  3,
        ['E'] =  4, ['F'] =  5, ['G'] =  6, ['H'] =  7,
        ['I'] =  8, ['J'] =  9, ['K'] = 10, ['L'] = 11,
        ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
        ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19,
        ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
        ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27,
        ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
        ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35,
        ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
        ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43,
        ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
        ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51,
        ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
        ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
        ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63
    };

    if (input_length % 4 != 0)
    {
        return NULL;
    }
    *output_length = input_length / 4 * 3;
    if ((input_length > 0) && (data[input_length - 1] == '='))
    {
        (*output_length)--;
    }
    if ((input_length > 1) && (data[input_length - 2] == '='))
    {
        (*output_length)--;
    }

    unsigned char *decoded = malloc(*output_length + 1);

    if (decoded == NULL)
    {
        return NULL;
    }
    for (size_t i = 0, j = 0; i < input_length;)
    {
        uint32_t sextet_a = table[(uint8_t)data[i++]];
        uint32_t sextet_b = table[(uint8_t)data[i++]];
        uint32_t sextet_c = table[(uint8_t)data[i++]];
        uint32_t sextet_d = table[(uint8_t)data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
                        + (sextet_b << 2 * 6)
                        + (sextet_c << 1 * 6)
                        + (sextet_d << 0 * 6);

        if (j < *output_length)
        {
            decoded[j++] = (triple >> 2 * 8) & 0xFF;
        }
        if (j < *output_length)
        {
            decoded[j++] = (triple >> 1 * 8) & 0xFF;
        }
        if (j < *output_length)
        {
            decoded[j++] = (triple >> 0 * 8) & 0xFF;
        }
    }
    decoded[*output_length] = '\0';
    return decoded;
}

