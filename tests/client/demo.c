/*!
 *  \brief     clux - json and json-schema library for C
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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <curl/curl.h>
#include <clux/json.h>

enum method
{
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    ALL 
};

struct data
{
    size_t length, size;
    char *text;
};

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

static int perform(CURL *curl, enum method method,
    const char *param, int id, const char *fields)
{
    const char *host = "http://localhost:1234";
    char url[128];

    switch (method)
    {
        case GET:
            printf("GET:%d\n", id);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
            snprintf(url, sizeof url, "%s/%s/%d", host, param, id);
            break;
        case POST:
            printf("POST:%s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(fields));
            snprintf(url, sizeof url, "%s/%s", host, param);
            break;
        case PUT:
            printf("PUT:%s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(fields));
            snprintf(url, sizeof url, "%s/%s/%d", host, param, id);
            break;
        case PATCH:
            printf("PATCH:%s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(fields));
            snprintf(url, sizeof url, "%s/%s/%d", host, param, id);
            break;
        case DELETE:
            printf("DELETE:%d\n", id);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
            snprintf(url, sizeof url, "%s/%s/%d", host, param, id);
            break;
        case ALL:
            printf("ALL:\n");
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
            snprintf(url, sizeof url, "%s/%s", host, param);
            break;
        default:
            return -1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    return curl_easy_perform(curl);
}

int main(void)
{
    struct data data = {0};

    setlocale(LC_CTYPE, "");
    srand((unsigned)time(NULL));
    curl_global_init(CURL_GLOBAL_DEFAULT);
    atexit(curl_global_cleanup);

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charset: utf-8");

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        perror("curl_easy_init");
        exit(EXIT_FAILURE);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, copy_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    int rc = EXIT_SUCCESS;

    for (int i = 0; i <= 10; i++)
    {
        int method = (i == 10) ? ALL : rand() % ALL;
        const char *param = "users";
        int id = rand() % 9 + 1;
        char fields[128];

        switch (method)
        {
            case POST:
            case PATCH:
                snprintf(fields, sizeof fields, "{\"name\":\"%c\"}", 'a' + id);
                break;
            case PUT:
                snprintf(fields, sizeof fields,
                    "{\"id\":%d,\"name\":\"%c\"}", id, 'a' + id);
                break;
            default:
                snprintf(fields, sizeof fields, "");
                break;
        }

        int res = perform(curl, method, param, id, fields);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform: %s\n", curl_easy_strerror(res));
            rc = EXIT_FAILURE;
            break;
        }
        if (data.length > 0)
        {
            json_error error;
            json *node = json_parse(data.text, &error);

            if (node == NULL)
            {
                json_print_error(&error);
            }
            else
            {
                json_print(node);
                json_free(node);
            }
            data.length = 0;
        }
    }
    free(data.text);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return rc;
}

