/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef HEADER_H
#define HEADER_H

enum
{
    HTTP_OK = 200,
    HTTP_NO_CONTENT = 204,
    HTTP_NOT_MODIFIED = 304,
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
static const char *http_no_cache =
    "HTTP/1.1 200 OK\r\n"
    "Cache-Control: no-cache\r\n"
    "ETag: \"%s\"\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_no_content =
    "HTTP/1.1 204 No Content%s\r\n\r\n";
static const char *http_not_modified =
    "HTTP/1.1 304 Not Modified%s\r\n\r\n";
static const char *http_bad_request =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_unauthorized =
    "HTTP/1.1 401 Unauthorized\r\n"
    "Set-Cookie: auth=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/; HttpOnly; SameSite=Strict\r\n"
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

