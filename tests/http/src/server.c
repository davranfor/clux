/*!
 *  \brief     C library for unixes
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
#include "access.h"
#include "reader.h"
#include "parser.h"
#include "server.h"

#define BUFFER_SIZE 32768

static char buffer[BUFFER_SIZE];

typedef struct pollfd conn_t;
typedef struct buffer pool_t;

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

static int server_listen(uint16_t port)
{
    struct sockaddr_in6 server;

    memset(&server, 0, sizeof server);
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(port);
    server.sin6_addr = in6addr_any;

    int fd, yes = 1, no = 0;

    if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof no) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
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

static void auth_attach(auth_t *auth)
{
    access_generate_key(auth);
}

static void conn_reset(conn_t *conn)
{
    close(conn->fd);
    conn->fd = -1;
    conn->events = 0;
}

static void auth_reset(auth_t *auth)
{
    auth->user = 0;
    auth->role = 0;
}

static void pool_reset(pool_t *pool)
{
    buffer_reset(pool);
}

static int conn_recv(conn_t *conn, pool_t *pool)
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

    int status = 1;

    if ((pool->length == 0) && ((status = reader_handle(buffer, rcvd)) == 1))
    {
        return status;
    }
    if (status != 0)
    {
        if (!buffer_append(pool, buffer, rcvd))
        {
            perror("buffer_append");
            return 0;
        }
        if (status != -1)
        {
            status = reader_handle(pool->text, pool->length);
        }
    }
    return status;
}

static int conn_send(conn_t *conn, pool_t *pool, const buffer_t *message)
{
    ssize_t bytes = send(conn->fd, message->text, message->length, 0);

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

    if (sent == message->length)
    {
        return 1;
    }
    if (pool != message)
    {
        if (!buffer_append(pool, message->text + sent, message->length - sent))
        {
            perror("buffer_append");
            return 0;
        }
    }
    else
    {
        buffer_delete(pool, 0, sent);
    }
    return -1;
}

static void conn_handle(conn_t *conn, auth_t *auth, pool_t *pool)
{
    if (!(conn->revents & ~(POLLIN | POLLOUT)))
    {
        const buffer_t *message = pool;

        if (conn->revents == POLLIN)
        {
            switch (conn_recv(conn, pool))
            {
                case 0:
                    goto reset;
                case -1:
                    return;
                default:
                    message = parser_handle(auth, pool->length ? pool->text : buffer);
                    pool_reset(pool);
                    break;
            }
        }
        if (message != NULL)
        {
            switch (conn_send(conn, pool, message))
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
    auth_reset(auth);
    pool_reset(pool);
}

static void conn_close(conn_t *conn, auth_t *auth, pool_t *pool)
{
    conn_reset(conn);
    auth_reset(auth);
    free(pool->text);
}

static void server_loop(uint16_t port)
{
    enum {server = 0, maxfds = MAX_CLIENTS + 1};
    conn_t conn[maxfds] = {0};
    auth_t auth[maxfds] = {0};
    pool_t pool[maxfds] = {0};

    conn_attach(&conn[server], server_listen(port));
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
                        auth_attach(&auth[client]);
                        done = 1;
                        break;
                    }
                }
                if (done == 0)
                {
                    close(fd);
                }
            }
        }
        for (nfds_t client = 1; client < maxfds; client++)
        {
            if (conn[client].revents)
            {
                conn_handle(&conn[client], &auth[client], &pool[client]);
            }
        }
    }
    for (nfds_t fd = 0; fd < maxfds; fd++)
    {
        if (conn[fd].fd != -1)
        {
            conn_close(&conn[fd], &auth[fd], &pool[fd]);
        }
    }
}

void server_run(uint16_t port)
{
    signal_connect();
    server_loop(port);
}

