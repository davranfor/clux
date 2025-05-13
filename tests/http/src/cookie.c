/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/rand.h>
#include "cookie.h"

//#define TOKEN_EXPIRATION 86400 // 1 day
#define TOKEN_EXPIRATION 5 // 30 seconds (for tests)
#define TOKEN_SIZE 65

int cookie_parse(cookie_t *cookie, const char *path, char *str)
{
    //Set-Cookie: auth=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/; Secure
    if ((str = strstr(str, "Cookie: auth=")))
    {
        str += 13;

        char *end = strchr(str, '\r');

        if (end == NULL)
        {
            return -1;
        }
        *end = '\0';

        int data[2] = {0};

        for (int i = 0; i < 2; i++)
        {
            char *ptr;

            data[i] = (int)strtol(str, &ptr, 10);
            if ((*ptr != ':') || (data[i] <= 0))
            {
                return -1;
            }
            str = ptr + 1;
        }
        if (end - str != TOKEN_SIZE - 1)
        {
            return -1;
        }
        cookie->user = data[0];
        cookie->role = data[1];
        cookie->token = str;
        return 1;
    }
    else if (!strcmp(path, "/login"))
    {
        cookie->token = "";
        return 1;
    }
    else
    {
        return 0;
    }
}

int cookie_create(int user, int role, char *cookie)
{
    unsigned char bytes[TOKEN_SIZE / 2];

    if (RAND_bytes(bytes, sizeof(bytes)) != 1)
    {
        return 0;
    }

    char *token = cookie + snprintf(cookie, COOKIE_SIZE, "%d:%d:", user, role);

    for (size_t i = 0; i < sizeof bytes; i++)
    {
        snprintf(token + (i * 2), 3, "%02x", bytes[i]);
    }
    token[TOKEN_SIZE - 1] = '\0';
    return 1;
}

/* Generate token HMAC (user:timestamp:hmac)
static void generate_token(char *token, int user)
{
    char data[128];
    time_t now = time(NULL);

    snprintf(data, sizeof data, "%d:%ld", user, now);

    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;

    HMAC(EVP_sha256(), SECRET_KEY, strlen(SECRET_KEY),
        (unsigned char *)data, strlen(data), hmac, &hmac_len);

    char hmac_hex[HMAC_SIZE];

    for (unsigned int i = 0; i < hmac_len; i++)
    {
        snprintf(&hmac_hex[i * 2], 3, "%02x", hmac[i]);
    }
    hmac_hex[HMAC_SIZE - 1] = '\0';
    snprintf(token, TOKEN_SIZE, "%s:%s", data, hmac_hex);
}

static int validate_token(char *token, int *user)
{
    *user = 0;

    char *part[3];

    part[0] = token;
    part[1] = strchr(part[0], ':');
    if (part[1] == NULL)
    {
        return 0;
    }
    part[1][0] = '\0';
    part[1]++;
    part[2] = strchr(part[1], ':');
    if (part[2] == NULL)
    {
        return 0;
    }
    part[2][0] = '\0';
    part[2]++;

    time_t timestamp = strtol(part[1], NULL, 10);

    if ((unsigned)time(NULL) - timestamp > TOKEN_EXPIRATION)
    {
        printf("Token expired\n");
        return 0;
    }

    char data[TOKEN_SIZE];

    snprintf(data, sizeof(data), "%s:%s", part[0], part[1]);

    unsigned char hmac[SHA256_DIGEST_LENGTH];
    unsigned int hmac_len;

    HMAC(EVP_sha256(), SECRET_KEY, strlen(SECRET_KEY),
        (unsigned char *)data, strlen(data), hmac, &hmac_len);

    char hmac_hex[HMAC_SIZE];

    for (unsigned int i = 0; i < hmac_len; i++)
    {
        snprintf(&hmac_hex[i * 2], 3, "%02x", hmac[i]);
    }
    printf("<%s:%s:%s>\n", part[0], part[1], part[2]);
    if (strcmp(hmac_hex, part[2]) != 0)
    {
        return 0;
    }
    *user = (int)strtol(part[0], NULL, 10);
    return 1;
}

const buffer_t *cookie_create(int user)
{
    (void)user;
    return NULL;
}

// Endpoint /api/login
static void handle_login(int client_socket, const char *body)
{
    // Simplified sample: usuario="admin", password="123"
    if (strstr(body, "username=admin") && strstr(body, "password=123"))
{
        char token[256];

        generate_auth_token(1, token); // user_id=1

        char response[1024];

        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Set-Cookie: auth_token=%s; Path=/; HttpOnly; Secure; SameSite=Strict\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{\"username\":\"admin\"}",
            token);

        //send(client_socket, response, strlen(response), 0);
    } else {
        const char *response =
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{\"error\":\"Credenciales inválidas\"}";
        //send(client_socket, response, strlen(response), 0);
    }
}

// Endpoint /api/check-auth
static void handle_check_auth(int client_socket, const char *headers)
{
    char *cookie = NULL; //extract_cookie(headers, "auth_token");
    int user_id;

    if (cookie && validate_token(cookie, &user_id))
    {
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{\"username\":\"admin\"}";
        //send(client_socket, response, strlen(response), 0);
    } else {
        const char *response =
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{}";
        //send(client_socket, response, strlen(response), 0);
    }
}

// Endpoint /api/fichar
static void handle_fichar(int client_socket, const char *headers)
{
    char *cookie = NULL; //extract_cookie(headers, "auth_token");
    int user_id;

    if (cookie && validate_token(cookie, &user_id))
    {
        // Registrar fichaje en BD (implementar según necesidad)
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{\"status\":\"success\"}";
        //send(client_socket, response, strlen(response), 0);
    }
    else
    {
        const char *response =
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: application/json\r\n"
            "\r\n"
            "{\"error\":\"No autorizado\"}";
        //send(client_socket, response, strlen(response), 0);
    }
}

// Endpoint /api/logout
static void handle_logout(int client_socket)
{
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Set-Cookie: auth_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly; Secure; SameSite=Strict\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"status\":\"success\"}";
    //send(client_socket, response, strlen(response), 0);
}
*/

