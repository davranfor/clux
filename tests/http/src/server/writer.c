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
    if (json_index(sections, "LOAD") != 0)
    {
        fprintf(stderr, "'app.json': 'LOAD' section must exist at index 0\n");
        exit(EXIT_FAILURE);
    }

    const json_t *tables = json_head(sections);

    for (unsigned i = 0; i < tables->size; i++)
    {
        json_t *entry = tables->child[i];

        if (entry->type != JSON_STRING)
        {
            fprintf(stderr, "'app.json': 'LOAD[%u]' must be a string\n", i);
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

static const char *get_sql(const char *method, const json_t *path)
{
    const json_t *section = json_find(sections, method);
    char *key = json_string(json_head(path));

    return json_string(json_search(section, &(json_t){.key = key}, NULL));
}

static int set_content(const json_t *content, sqlite3_stmt *stmt, int id)
{
    for (unsigned i = 0; i < content->size; i++)
    {
        const json_t *child = content->child[i];
        int result = SQLITE_OK;

        switch (child->type)
        {
            case JSON_STRING:
                result = sqlite3_bind_text(stmt, id++, child->string, -1, SQLITE_STATIC);
                break;
            case JSON_INTEGER:
                result = sqlite3_bind_int64(stmt, id++, (int64_t)child->number);
                break;
            case JSON_REAL:
                result = sqlite3_bind_double(stmt, id++, child->number);
                break;
            case JSON_TRUE:
                result = sqlite3_bind_int(stmt, id++, 1);
                break;
            case JSON_FALSE:
                result = sqlite3_bind_int(stmt, id++, 0);
                break;
            case JSON_NULL:
                result = sqlite3_bind_null(stmt, id++);
                break;
            default:
                return 0;
        }
        if (result != SQLITE_OK)
        {
            return 0;
        }
    }
    return id;
}

static int db_get(const json_t *request)
{
    const json_t *path = json_find(request, "path");
    const char *sql = get_sql("GET", path);

    if (sql == NULL)
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

    int size = (int)json_size(path);

    for (int i = 1; i < size; i++)
    {
        const char *text = json_text(path->child[i]);

        if (sqlite3_bind_text(stmt, i, text, -1, SQLITE_STATIC) != SQLITE_OK)
        {
            fprintf(stderr, "%s\n", sqlite3_errmsg(db));
            buffer_write(&buffer, sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return HTTP_BAD_REQUEST;
        }
    }

    int result = HTTP_NO_CONTENT;
    int step;

    while ((step = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *text = sqlite3_column_text(stmt, 0);

        buffer_write(&buffer, (const char *)text);
        result = HTTP_OK;
    }
    if (step != SQLITE_DONE)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        buffer_write(&buffer, sqlite3_errmsg(db));
        result = HTTP_BAD_REQUEST;
    }
    sqlite3_finalize(stmt);
    return result;
}

static int db_post(const json_t *request)
{
    const json_t *content = json_find(request, "content");
    const json_t *path = json_find(request, "path");
    const char *sql = get_sql("POST", path);

    if ((sql == NULL) || (content == NULL))
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
    if (!set_content(content, stmt, 1))
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

static int db_put(const json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int db_patch(json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int db_delete(json_t *request)
{
    (void)request;
    return HTTP_NO_CONTENT;
}

static int db_handle(json_t *request)
{
    switch (get_method(request->key))
    {
        case GET:
            return db_get(request);
        case POST:
            return db_post(request);
        case PUT:
            return db_put(request);
        case PATCH:
            return db_patch(request);
        case DELETE:
            return db_delete(request);
        default:
            return HTTP_NO_CONTENT;
    }
}

static void cleanup(json_t *request)
{
    json_delete(json_find(request, "content"));
    request->size = 0;
}

static const buffer_t *process(int header)
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
    int result = schema_validate(request, &buffer);

    if (result == HTTP_OK)
    {
        buffer_reset(&buffer);
        result = db_handle(request);
        cleanup(request);
    }
    if (result != HTTP_NO_CONTENT)
    {
        return process(result);
    }
    return static_no_content();
}

