/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef HEADERS_H
#define HEADERS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *http_html_ok =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_json_ok =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_no_content =
    "HTTP/1.1 204 No Content\r\n\r\n";
static const char *http_not_found =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n\r\n"
    "404 Not Found";
static const char *http_method_not_allowed =
    "HTTP/1.1 405 Method Not Allowed\r\n"
    "Allow: GET, POST, PUT, DELETE, PATCH\r\n"
    "Content-Length: 0\r\n\r\n";
#pragma GCC diagnostic pop

#endif

