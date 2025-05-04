/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef HEADERS_H
#define HEADERS_H

enum
{
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_NO_CONTENT = 204,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_SERVER_ERROR = 500
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *http_ok =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_created =
    "HTTP/1.1 201 Created\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_no_content =
    "HTTP/1.1 204 No Content\r\n\r\n";
static const char *http_bad_request =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_unauthorized =
    "HTTP/1.1 401 Unauthorized\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_forbidden =
    "HTTP/1.1 403 Forbidden\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_not_found =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_server_error =
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
#pragma GCC diagnostic pop

#endif

