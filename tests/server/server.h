#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include "buffer.h" 

void server_loop(
    uint16_t,
    int (*)(const char *, size_t),
    void (*)(struct poolfd *, char *, size_t)
);

#endif

