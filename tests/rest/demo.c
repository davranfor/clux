/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
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

Compile and run with:
CFLAGS="-std=c11 -Wpedantic -Wall -Wextra -O2" LDLIBS="-lcurl -lclux" make demo && ./demo

If you need a json server for testing: 

npm install -g json-server

Create a json file (i.e. db.json):

{
  "users": [
  ]
}

Run the server:

json-server --watch db.json

Test the server on the browser: http://localhost:3000

More info: https://www.npmjs.com/package/json-server?activeTab=readme 
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

static int perform(CURL *curl, enum method method, int id, const char *fields)
{
    char url[128];

    switch (method)
    {
        case GET:
            printf("GET:%d\n", id);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
            snprintf(url, sizeof url, "http://localhost:3000/users/%d", id);
            break;
        case POST:
            printf("POST:%s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            snprintf(url, sizeof url, "http://localhost:3000/users");
            break;
        case PUT:
            printf("PUT:%s\n", fields);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
            snprintf(url, sizeof url, "http://localhost:3000/users/%d", id);
            break;
        case DELETE:
            printf("DELETE:%d\n", id);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
            snprintf(url, sizeof url, "http://localhost:3000/users/%d", id);
            break;
        case ALL:
            printf("ALL:\n");
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
            snprintf(url, sizeof url, "http://localhost:3000/users");
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
        int id = rand() % 9 + 1;
        char fields[128];

        snprintf(fields, sizeof fields, "{\"id\": %d,\"name\": \"Item #%d\"}", id, id);

        int res = perform(curl, method, id, fields);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform: %s\n", curl_easy_strerror(res));
            rc = EXIT_FAILURE;
            break;
        }

        json_error error;
        json *node = json_parse(data.text, &error);

        if (node == NULL)
        {
            if (strncmp(data.text, "Error:", 6) == 0)
            {
                data.text[strcspn(data.text, "\n")] = '\0';
                puts(data.text);
            }
            else
            {
                json_print_error(&error);
            }
        }
        json_print(node);
        json_free(node);
        data.length = 0;
    }
    free(data.text);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return rc;
}

