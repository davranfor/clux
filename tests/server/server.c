/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "config.h"
#include "request.h"
#include "server.h"

#define BUFFER_SIZE 32768

static char buffer[BUFFER_SIZE];

typedef struct pollfd conn_t;

static volatile sig_atomic_t stop;

static void signal_handle(int signum)
{
    fprintf(stderr, "\nCaught signal %d (SIGINT)\n", signum);
    stop = 1;
}

static void signal_connect(void)
{
    struct sigaction sa;

    sa.sa_handler = signal_handle;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

static int unblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
    {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int server_socket(uint16_t port)
{
    struct sockaddr_in server;

    memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    int fd, opt = 1;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (unblock(fd) == -1)
    {
        perror("unblock");
        exit(EXIT_FAILURE);
    }
    if (bind(fd, (struct sockaddr *)&server, sizeof server) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(fd, MAX_CLIENTS) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return fd;
}

static void conn_attach(conn_t *conn, int fd)
{
    conn->fd = fd;
    conn->events = POLLIN;
}

static void conn_reset(conn_t *conn)
{
    close(conn->fd);
    conn->fd = -1;
    conn->events = 0;
}

static ssize_t conn_recv(conn_t *conn, pool_t *pool)
{
    ssize_t bytes = recv(conn->fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes == 0)
    {
        return 0;
    }
    if (bytes == -1)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            return -1;
        }
        perror("recv");
        return 0;
    }

    size_t rcvd = (size_t)bytes;

    buffer[rcvd] = '\0';

    ssize_t request = 1;

    if ((pool->text == NULL) && ((request = request_handle(buffer, rcvd)) == 1))
    {
        pool_bind(pool, buffer, rcvd);
    }
    else if (request != 0)
    {
        if (!pool_put(pool, buffer, rcvd))
        {
            perror("pool_put");
            return 0;
        }
        if (request != -1)
        {
            request = request_handle(pool->text, pool->length);
        }
    }
    return request;
}

static ssize_t conn_send(conn_t *conn, pool_t *pool)
{
    char *text = pool->text + pool->sent;
    size_t length = pool->length - pool->sent;
    ssize_t bytes = send(conn->fd, text, length, 0);

    if (bytes == 0)
    {
        return 0;
    }
    if (bytes == -1)
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            perror("send");
            return 0;
        }
        bytes = 0;
    }

    size_t sent = (size_t)bytes;

    if (sent == length)
    {
        return bytes;
    }
    if (pool->type == POOL_BUFFERED)
    {
        if (!pool_put(pool, text + sent, length - sent))
        {
            perror("pool_put");
            return 0;
        }
    }
    else
    {
        pool_sync(pool, sent);
    }
    return -1;
}

static void conn_handle(conn_t *conn, pool_t *pool)
{
    if (!(conn->revents & ~(POLLIN | POLLOUT)))
    {
        if (conn->revents == POLLIN)
        {
            switch (conn_recv(conn, pool))
            {
                case 0:
                    goto reset;
                case -1:
                    return;
                default:
                    puts("");
                    puts(pool->text);
                    request_reply(pool, buffer, BUFFER_SIZE);
                    break;
            }
        }
        if (pool->text != NULL)
        {
            switch (conn_send(conn, pool))
            {
                case 0:
                    goto reset;
                case -1:
                    conn->events |= POLLOUT;
                    return;
                default:
                    conn->events &= ~POLLOUT;
                    pool_reset(pool);
                    return;
            }
        }
    }
reset:
    conn_reset(conn);
    pool_reset(pool);
}

static void conn_close(conn_t *conn, pool_t *pool)
{
    conn_reset(conn);
    pool_reset(pool);
}

static void server_loop(uint16_t port)
{
    enum {server = 0, maxfds = MAX_CLIENTS + 1};
    conn_t conn[maxfds] = {0};
    pool_t pool[maxfds] = {0};

    conn_attach(&conn[server], server_socket(port));
    for (nfds_t client = 1; client < maxfds; client++)
    {
        conn[client].fd = -1;
    }
    while (!stop)
    {
        if (poll(conn, maxfds, -1) == -1)
        {
            perror("poll");
            break;
        }
        if (conn[server].revents & POLLIN)
        {
            int fd = accept(conn[server].fd, NULL, NULL);

            if (fd == -1)
            {
                if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
                {
                    perror("accept");
                    break;
                }
            }
            else
            {
                int done = 0;

                for (nfds_t client = 1; client < maxfds; client++)
                {
                    if (conn[client].fd == -1)
                    {
                        if (unblock(fd) == -1)
                        {
                            perror("unblock");
                            break;
                        }
                        conn_attach(&conn[client], fd);
                        done = 1;
                        break;
                    }
                }
                if (!done)
                {
                    close(fd);
                }
            }
        }
        for (nfds_t client = 1; client < maxfds; client++)
        {
            if (conn[client].revents)
            {
                conn_handle(&conn[client], &pool[client]);
            }
        }
    }
    for (nfds_t fd = 0; fd < maxfds; fd++)
    {
        if (conn[fd].fd != -1)
        {
            conn_close(&conn[fd], &pool[fd]);
        }
    }
}

void server_run(uint16_t port)
{
    signal_connect();
    server_loop(port);
}

