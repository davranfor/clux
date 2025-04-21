/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <clux/clib.h>
#include <clux/json.h>
#include <clux/json_private.h>
#include <sqlite3.h>
#include "headers.h"
#include "schema.h"
#include "static.h"
#include "writer.h"

static buffer_t buffer;
static json_t *sections;
static sqlite3 *db;

static void load_db(void)
{
    const json_t *tables = json_find(sections, "LOAD");

    if (tables == NULL)
    {
        fprintf(stderr, "'app.json': 'LOAD' section must exist\n");
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < tables->size; i++)
    {
        json_t *entry = tables->child[i];

        if (entry->type != JSON_STRING)
        {
            fprintf(stderr, "'app.json': 'tables[%u]' must be a string\n", i);
            exit(EXIT_FAILURE);
        }

        char *err = NULL;

        if (sqlite3_exec(db, entry->string, NULL, NULL, &err) != SQLITE_OK)
        {
            fprintf(stderr, "%s\n%s\n", entry->string, err);
            sqlite3_free(err);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 1; i < sections->size; i++)
    {
        json_sort(sections->child[i], NULL);
    }
}

static void load(void)
{
    json_error_t error = {0};

    printf("Loading 'app.json'\n");
    if (!(sections = json_parse_file("app.json", &error)))
    {
        json_print_error(&error);
        exit(EXIT_FAILURE);
    }
    printf("Loading 'app.db'\n");
    if (sqlite3_open("app.db", &db))
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    load_db();
}

static void unload(void)
{
    buffer_clean(&buffer);
    json_free(sections);
    sqlite3_close(db);
}

void writer_load(void)
{
    atexit(unload);
    load();
}

void writer_reload(void)
{
    unload();
    load();
}

static int validate(const json_t *rules, json_t *request, const char *path)
{
    unsigned content_id = json_index(request, "content");
    json_t *content = request->child[content_id];
    const char *text = "null";

    if (content->type == JSON_STRING)
    {
        text = content->string;
    }

    json_error_t error;
    json_t *node = json_parse(text, &error);

    if (node && json_set_key(node, "content"))
    {
        request->child[content_id] = node;
    }
    else
    {
        buffer_write(&buffer, "Error parsing content");
        json_print_error(&error);
        json_free(node);
        return 0;
    }

    json_t entry =
    {
        .type = JSON_OBJECT,
        .child = (json_t *[]){request},
        .size = 1
    };

    int rc = schema_validate(rules, &entry, &buffer, path);

    request->child[content_id] = content;
    json_free(node);
    return rc;
}

enum method {GET, POST, PUT, PATCH, DELETE, METHODS};

static enum method get_method(const char *message)
{
    const char *name[] =
    {
        "GET", "POST", "PUT", "PATCH", "DELETE"
    };
    enum method method;

    for (method = 0; method < METHODS; method++)
    {
        if (!strcmp(message, name[method]))
        {
            break;
        }
    }
    return method;
}

static const char *get_sql(const json_t *request, const char *method)
{
    const json_t *section = json_find(sections, method);
    char *path = json_text(json_head(json_find(request, "path")));

    return json_string(json_search(section, &(json_t){.key = path}, NULL));
}

static int api_get(const json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int api_post(const json_t *request)
{
    const char *content = json_string(json_find(request, "content"));
    const char *sql = get_sql(request, "POST");

    if ((content == NULL) || (sql == NULL))
    {
        const char *error = "Query can't be performed";

        json_write_line(request, stderr);
        fprintf(stderr, "%s\n", error);
        buffer_write(&buffer, error);
        return HTTP_SERVER_ERROR;
    }

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        buffer_write(&buffer, sqlite3_errmsg(db));
        return HTTP_BAD_REQUEST;
    }
    if (sqlite3_bind_text(stmt, 1, content, -1, SQLITE_STATIC) != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        buffer_write(&buffer, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return HTTP_BAD_REQUEST;
    }
    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        buffer_write(&buffer, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return HTTP_BAD_REQUEST;
    }

    sqlite3_int64 id = sqlite3_last_insert_rowid(db);

    buffer_format(&buffer, "{\"id\": %lld}", id);
    sqlite3_finalize(stmt);
    return HTTP_CREATED;
}

static int api_put(const json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int api_patch(json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int api_delete(json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int api_handle(json_t *request)
{
    switch (get_method(request->key))
    {
        case GET:
            return api_get(request);
        case POST:
            return api_post(request);
        case PUT:
            return api_put(request);
        case PATCH:
            return api_patch(request);
        case DELETE:
            return api_delete(request);
        default:
            return HTTP_NO_CONTENT;
    }
}

static const buffer_t *response(int header)
{
    const char *code, *type;
    char headers[128];

    switch (header)
    {
        case HTTP_OK:
            code = http_ok;
            type = "application/json";
            break;
        case HTTP_CREATED:
            code = http_created;
            type = "application/json";
            break;
        case HTTP_NOT_FOUND:
            code = http_not_found;
            type = "text/plain";
            break;
        case HTTP_SERVER_ERROR:
            code = http_server_error;
            type = "text/plain";
            break;
        default:
            code = http_bad_request;
            type = "text/plain";
            break;
    }
    snprintf(headers, sizeof headers, code, type, buffer.length);
    buffer_insert(&buffer, 0, headers, strlen(headers));
    return buffer.length ? &buffer : NULL;
}

const buffer_t *writer_handle(json_t *request)
{
    json_print(request);

    const char *path = json_text(json_head(json_find(request, "path"))); 
    const json_t *schema = schema_get(path);

    if (schema == NULL)
    {
        return static_bad_request();
    }
    /* Validate request */
    buffer_reset(&buffer);

    int result = validate(schema, request, path);

    if (result != HTTP_OK)
    {
        return response(result);
    }
    /* Apply db request */
    buffer_reset(&buffer);
    result = api_handle(request);
    if (result != HTTP_NO_CONTENT)
    {
        return response(result);
    }
    return static_no_content();
}

