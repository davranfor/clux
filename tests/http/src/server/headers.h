/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#ifndef HEADERS_H
#define HEADERS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *http_ok =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_no_content =
    "HTTP/1.1 204 No Content\r\n\r\n";
static const char *http_bad_request =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
static const char *http_not_found =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n\r\n";
#pragma GCC diagnostic pop

#endif

