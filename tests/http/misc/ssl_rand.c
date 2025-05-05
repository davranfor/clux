#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>

#define AUTH_KEY_SIZE 65 

void generate_key(char *key)
{
    unsigned char bin[AUTH_KEY_SIZE / 2];

    if (RAND_bytes(bin, sizeof bin) != 1)
    {
        fprintf(stderr, "Error generating random bytes\n");
        exit(EXIT_FAILURE);
    }
    // Convert to hex
    for (size_t i = 0; i < sizeof bin; i++)
    {
        snprintf(key + (i * 2), 3, "%02x", bin[i]);
    }
    key[AUTH_KEY_SIZE - 1] = '\0';
}

int main(void)
{
    char key[AUTH_KEY_SIZE];

    generate_key(key);
    printf("Key: %s\n", key);
    return 0;
}

