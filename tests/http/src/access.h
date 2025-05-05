/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef ACCESS_H
#define ACCESS_H

#define AUTH_KEY_SIZE 65 

typedef struct
{
    int user, role;
    char key[AUTH_KEY_SIZE];
} auth_t;

void access_generate_key(auth_t *);
int access_handle(auth_t *);

#endif

