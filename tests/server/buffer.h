#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_SIZE 32768

struct poolfd
{
    char *data;
    size_t size, sent;
    unsigned type;
};

enum {POOL_BUFFERED = 1, POOL_ALLOCATED};

char *pool_set(struct poolfd *, char *, size_t);
char *pool_add(struct poolfd *, const char *, size_t);
void pool_sync(struct poolfd *, size_t);
void pool_reset(struct poolfd *);

#endif

