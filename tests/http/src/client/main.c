#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <clux/clib.h>
#include <clux/json.h>
#include "config.h"
#include "common.h"
#include "reader.h"

#define CLIENT_TIMEOUT 60
#define BUFFER_SIZE 512
 
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int client_connect(void *server)
{
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    struct timeval tv = {CLIENT_TIMEOUT, 0};

    if ((setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv) == -1) ||
        (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) == -1))
    {
        perror("setsockopt");
        return -1;
    }
    if (connect(fd, (struct sockaddr *)server, sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect");
        return -1;
    }
    return fd;
}

static json_t *users;

static void delete_users(void)
{
    json_delete(users);
}

enum { GET, POST, PUT, PATCH, DELETE, METHODS };

static char *pick_user(buffer_t *buffer, char *data, size_t size)
{
    size_t id = (size_t)rand() % json_size(users);
    int method = rand() % METHODS;
    const char *table = "users";
    char head[256], body[256];

    switch (method)
    {
        case POST:
        case PUT:
        case PATCH:
            snprintf(body, sizeof body,
                "{\"name\": \"%s\", \"email\": \"%s\"}",
                json_text(json_find(json_at(users, id), "name")),
                json_text(json_find(json_at(users, id), "email"))
            );
            break;
        default:
            body[0] = '\0';
            break;
    }
    switch (method)
    {
        case GET:
            snprintf(data, size, "GET %zu", id);
            snprintf(head, sizeof head,
                "GET /api/%s/%zu HTTP/1.1\r\n\r\n",
                table, id);
            break;
        case POST:
            snprintf(data, size, "POST %s", body);
            snprintf(head, sizeof head,
                "POST /api/%s HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n\r\n",
                table, strlen(body));
            break;
        case PUT:
            snprintf(data, size, "PUT %s", body);
            snprintf(head, sizeof head,
                "PUT /api/%s/%zu HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n\r\n",
                table, id, strlen(body));
            break;
        case PATCH:
            snprintf(data, size, "PATCH %zu %s", id, body);
            snprintf(head, sizeof head,
                "PATCH /api/%s/%zu HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n\r\n",
                table, id, strlen(body));
            break;
        case DELETE:
            snprintf(data, size, "DELETE %zu", id);
            snprintf(head, sizeof head,
                "DELETE /api/%s/%zu HTTP/1.1\r\n\r\n",
                table, id);
            break;
    }
    buffer_write(buffer, head);
    buffer_write(buffer, body);
    return buffer->text;
}

static int send_handle(int fd, const buffer_t *pool)
{
    size_t sent = 0;

    while (sent < pool->length)
    {
        ssize_t bytes = send(fd, pool->text + sent, pool->length - sent, 0);

        if (bytes == -1)
        {
            perror("send");
            return 0;
        }
        if (bytes == 0)
        {
            return 0;
        }
        sent += (size_t)bytes;
    }
    return 1;
}

static char *recv_handle(int fd, char *buffer, buffer_t *pool)
{
    for (;;)
    {
        ssize_t bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes == -1)
        {
            perror("recv");
            return NULL;
        }
        if (bytes == 0)
        {
            return NULL;
        }

        size_t rcvd = (size_t)bytes;

        buffer[rcvd] = '\0';

        char *text = NULL;
        int status = 1;

        if ((pool->length == 0) && ((status = reader_handle(buffer, rcvd)) == 1))
        {
            text = buffer;
        }
        else if (status != 0)
        {
            if (!buffer_append(pool, buffer, rcvd))
            {
                perror("buffer_append");
                return NULL;
            }
            if (status != -1)
            {
                status = reader_handle(pool->text, pool->length);
                text = pool->text;
            }
        }
        if (status == 0)
        {
            return NULL;
        }
        if (status == 1)
        {
            return text;
        }
    }
    return NULL;
}

static void *handler(void *server)
{
    int fd;

    if ((fd = client_connect(server)) == -1)
    {
        return NULL;
    }

    char buffer[BUFFER_SIZE];
    buffer_t pool = {0};

    for (size_t i = 0; i < 10; i++)
    {
        char data[512];

        if (!pick_user(&pool, data, sizeof data))
        {
            goto stop;
        }
        if (!send_handle(fd, &pool))
        {
            goto stop;
        }
        buffer_reset(&pool);

        const char *rcvd = recv_handle(fd, buffer, &pool);

        if (rcvd == NULL)
        {
            goto stop;
        }
        pthread_mutex_lock(&mutex);
        printf("----------------------------------------------------------------------\n%s\n%s\n", data, rcvd);
        pthread_mutex_unlock(&mutex);
        buffer_reset(&pool);
    }
stop:
    close(fd);
    free(pool.text);
    return NULL;
}

int main(int argc, char *argv[])
{
    if ((argc == 2) && (strcmp(argv[1], "-h") == 0))
    {
        printf("Usage: %s [address] [port]\n", argv[0]);
        return 0;
    }

    srand((unsigned)time(NULL));
    setlocale(LC_NUMERIC, "C");
    atexit(delete_users);

    const char *path = "samples/users.json"; 
    json_error_t error;

    if (!(users = json_parse_file(path, &error)))
    {
        fprintf(stderr, "%s\n", path);
        json_print_error(&error);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;

    memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;

    const char *addr = argc > 1 ? argv[1] : SERVER_ADDR;

    if (inet_pton(AF_INET, addr, &server.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    }

    uint16_t port = argc > 2 ? port_number(argv[2]) : SERVER_PORT;

    if (port == 0)
    {
        fprintf(stderr, "Invalid port\n");
        exit(EXIT_FAILURE);
    }
    server.sin_port = htons(port);

    pthread_t thread[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (pthread_create(&thread[i], NULL, handler, &server) != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (pthread_join(thread[i], NULL) != 0)
        {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

