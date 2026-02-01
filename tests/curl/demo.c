/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/**
API REST

For this example you may need to install the development libraries for curl

On debian:
sudo apt install libcurl4-gnutls-dev

On macos:
brew install curl

Run the server on test/server

Compile and run with:
CFLAGS="-std=c11 -Wpedantic -Wall -Wextra -O2" LDLIBS="-lcurl -lclux" make demo && ./demo

Manual GET:
curl -v -G "http://127.0.0.1:1234/api/users" --data-urlencode "name=John" --data-urlencode "surname=Doe"

*/

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <curl/curl.h>
#include <clux/json.h>

enum method { GET, POST, PUT, PATCH, DELETE, METHODS };

struct data { char *text; size_t length, size; };

// cppcheck-suppress constParameterCallback
static size_t copy_data(void *text, size_t sz, size_t elems, void *stream)
{
    struct data *data = stream;
    size_t length = sz * elems;
    size_t size = data->length + length + 1;

    if (size > data->size)
    {
        char *temp = realloc(data->text, size);

        if (temp == NULL)
        {
            return 0;
        }
        data->text = temp;
        data->size = size;
    }
    ((char *)memcpy(data->text + data->length, text, length))[length] = '\0';
    data->length += length;
    return length;
}

static CURLcode perform(CURL *curl, int method,
    const char *param, size_t id, const char *fields)
{
    const char *host = "http://127.0.0.1:1234";
    char url[128];

    switch (method)
    {
        case GET:
            printf("GET %zu\n", id);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
            snprintf(url, sizeof url, "%s/api/%s/%zu", host, param, id);
            break;
        case POST:
            printf("POST %s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(fields));
            snprintf(url, sizeof url, "%s/api/%s", host, param);
            break;
        case PUT:
            printf("PUT %s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(fields));
            snprintf(url, sizeof url, "%s/api/%s/%zu", host, param, id);
            break;
        case PATCH:
            printf("PATCH %zu %s\n", id, fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(fields));
            snprintf(url, sizeof url, "%s/api/%s/%zu", host, param, id);
            break;
        case DELETE:
            printf("DELETE %zu\n", id);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            snprintf(url, sizeof url, "%s/api/%s/%zu", host, param, id);
            break;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    return curl_easy_perform(curl);
}

static int request(size_t id, const json_t *users, struct data *data)
{
    int method = rand() % METHODS;
    const char *param = "users";
    char fields[128];
    int rc = 1;

    switch (method)
    {
        case POST:
        case PATCH:
            snprintf(fields, sizeof fields,
                "{\"name\": \"%s\", \"surname\": \"%s\"}",
                json_text(json_find(json_at(users, id), "name")),
                json_text(json_find(json_at(users, id), "surname"))
            );
            break;
        case PUT:
            snprintf(fields, sizeof fields,
                "{\"id\": %zu, \"name\": \"%s\", \"surname\": \"%s\"}",
                id,
                json_text(json_find(json_at(users, id), "name")),
                json_text(json_find(json_at(users, id), "surname"))
            );
            break;
        default:
            snprintf(fields, sizeof fields, "%s", "");
            break;
    }

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        perror("curl_easy_init");
        exit(EXIT_FAILURE);
    }

    struct curl_slist *headers = NULL;

    if ((method != GET) && (method != DELETE))
    {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, copy_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode code = perform(curl, method, param, id, fields);

    if (code != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform: %s\n", curl_easy_strerror(code));
        rc = 0;
    }
    else if (data->length > 0)
    {
        json_error_t error;
        json_t *node = json_parse(data->text, &error);

        if (node == NULL)
        {
            json_print_error(&error);
        }
        else
        {
            json_print(node);
            json_delete(node);
        }
        data->length = 0;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return rc;
}

int main(void)
{
    struct data data = { 0 };

    srand((unsigned)time(NULL));
    setlocale(LC_NUMERIC, "C");

    json_error_t error;
    json_t *users = json_parse_file("users.json", &error);

    if (users == NULL)
    {
        json_print_error(&error);
        exit(EXIT_FAILURE);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    atexit(curl_global_cleanup);

    for (size_t i = 0, n = json_size(users); i < n; i++)
    {
        size_t id = (size_t)rand() % n;

        if (!request(id, users, &data))
        {
            break;
        }
    }
    json_delete(users);
    free(data.text);
    return 0;
}

