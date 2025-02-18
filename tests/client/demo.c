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

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 1234 

#define MAX_CLIENTS 10
#define CLIENT_TIMEOUT 60

#define HEADERS_MAX_LENGTH 4096

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
        case PATCH:
            snprintf(body, sizeof body,
                "{\"name\": \"%s\", \"surname\": \"%s\"}",
                json_text(json_find(json_at(users, id), "name")),
                json_text(json_find(json_at(users, id), "surname"))
            );
            break;
        case PUT:
            snprintf(body, sizeof body,
                "{\"id\": %zu, \"name\": \"%s\", \"surname\": \"%s\"}",
                id,
                json_text(json_find(json_at(users, id), "name")),
                json_text(json_find(json_at(users, id), "surname"))
            ); 
            break;
        default:
            snprintf(body, sizeof body, "%s", "");
            break;
    }
    switch (method)
    {
        case GET:
            snprintf(data, size, "GET %zu", id);
            snprintf(head, sizeof head,
                "GET /%s/%zu HTTP/1.1\r\n"
                "Content-Type: application/json\r\n\r\n",
                table, id);
            break;
        case POST:
            snprintf(data, size, "POST %s", body);
            snprintf(head, sizeof head,
                "POST /%s HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n\r\n",
                table, strlen(body));
            break;
        case PUT:
            snprintf(data, size, "PUT %s", body);
            snprintf(head, sizeof head,
                "PUT /%s/%zu HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n\r\n",
                table, id, strlen(body));
            break;
        case PATCH:
            snprintf(data, size, "PATCH %zu %s", id, body);
            snprintf(head, sizeof head,
                "PATCH /%s/%zu HTTP/1.1\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n\r\n",
                table, id, strlen(body));
            break;
        case DELETE:
            snprintf(data, size, "DELETE %zu", id);
            snprintf(head, sizeof head,
                "DELETE /%s/%zu HTTP/1.1\r\n"
                "Content-Type: application/json\r\n\r\n",
                table, id);
            break;
    }
    buffer_write(buffer, head);
    buffer_write(buffer, body);
    return buffer->text;
}

static int recv_status(char *message, size_t length)
{
    char *delimiter = strstr(message, "\r\n\r\n");

    if (delimiter == NULL)
    {
        return length > HEADERS_MAX_LENGTH ? 0 : -1;
    }
    delimiter[0] = '\0';

    const char *content_length_mark = strstr(message, "Content-Length:");

    delimiter[0] = '\r';
    if (content_length_mark == NULL)
    {
        return delimiter[4] == '\0';
    }

    size_t headers_length = (size_t)(delimiter - message) + 4;
    size_t content_length = strtoul(content_length_mark + 15, NULL, 10);
    size_t request_length = headers_length + content_length;

    return length < request_length ? -1 : length == request_length;
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

    for (size_t i = 0, n = json_size(users); i < n; i++)
    {
        char data[512];

        if (!pick_user(&pool, data, sizeof data))
        {
            goto stop;
        }

        size_t sent = 0;

        while (sent < pool.length)
        {
            ssize_t bytes = send(fd, pool.text + sent, pool.length - sent, 0);

            if (bytes == -1)
            {
                perror("send");
                goto stop;
            }
            if (bytes == 0)
            {
                goto stop;
            }
            sent += (size_t)bytes;
        }

        buffer_reset(&pool);

        char *text = NULL;

        for (;;)
        {
            ssize_t bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);

            if (bytes == -1)
            {
                perror("recv");
                goto stop;
            }
            if (bytes == 0)
            {
                goto stop;
            }

            size_t rcvd = (size_t)bytes;

            buffer[rcvd] = '\0';

            int status = 1;

            if ((pool.length == 0) && ((status = recv_status(buffer, rcvd)) == 1))
            {
                text = buffer;
            }
            else if (status != 0)
            {
                if (!buffer_append(&pool, buffer, rcvd))
                {
                    perror("buffer_append");
                    goto stop;
                }
                if (status != -1)
                {
                    status = recv_status(pool.text, pool.length);
                    text = pool.text;
                }
            }
            if (status == 0)
            {
                goto stop;
            }
            if (status == 1)
            {
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        puts(data);
        puts(text);
        pthread_mutex_unlock(&mutex);
        buffer_reset(&pool);
    }
stop:
    close(fd);
    free(pool.text);
    return NULL;
}

static uint16_t port_number(const char *str)
{
    char *end;
    unsigned long result = strtoul(str, &end, 10);

    if ((result > 65535) || (end[strspn(end, " \f\n\r\t\v")] != '\0'))
    {
        return 0;
    }
    return (uint16_t)result;
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

    json_error_t error;

    if (!(users = json_parse_file("users.json", &error)))
    {
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
    json_delete(users);
    return 0;
}

