/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/**
Download, parse and display a json from jsonplaceholder.typicode.com

For this example you may need to install the development libraries for curl

On debian:
sudo apt install libcurl4-gnutls-dev
 
On macos:
brew install curl

Compile and run with:
CFLAGS="-std=c11 -Wpedantic -Wall -Wextra -O2" LDLIBS="-lcurl -lclux" make demo && ./demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <clux/json.h>

struct data
{
    size_t length, size;
    char *text;
};

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
    memcpy(data->text + data->length, text, length);
    data->length += length;
    data->text[data->length] = '\0';
    return length;
}

int main(void)
{
    struct data data = {0};

    curl_global_init(CURL_GLOBAL_DEFAULT);

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "charset: utf-8");

    CURL *curl = curl_easy_init();

    if (curl == NULL)
    {
        perror("curl_easy_init");
        goto cleanup;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, copy_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

    char url[128];

    for (int i = 1; i <= 10; i++)
    {
        snprintf(url, sizeof url, "https://jsonplaceholder.typicode.com/todos/%d", i);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (curl_easy_perform(curl) != CURLE_OK)
        {
            perror("curl_easy_perform");
            goto cleanup;
        }
        
        json_error error;
        json *node = json_parse(data.text, &error);

        if (node != NULL)
        {
            json_print(node);
            json_free(node);
        }
        else
        {
            json_print_error(&error);
        }
        data.length = 0;
    }
cleanup:
    free(data.text);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}

