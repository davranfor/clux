/*!
 *  \brief     C library for unixes
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
#include "header.h"
#include "schema.h"
#include "cookie.h"
#include "writer.h"

static sqlite3 *db;

static buffer_t buffer;

static json_t *catalog;
static json_t *session;
static json_t *queries;

static cookie_t cookie;
static char cookie_str[COOKIE_SIZE];

static void load_db(void)
{
    const json_t *tables = json_find(catalog, "load");

    if (tables == NULL)
    {
        fprintf(stderr, "Catalog: 'load' section must exist\n");
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < tables->size; i++)
    {
        json_t *entry = tables->child[i];

        if (entry->type != JSON_STRING)
        {
            fprintf(stderr, "Catalog': 'load[%u]' must be a string\n", i);
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
    if (!(session = json_find(catalog, "session")))
    {
        fprintf(stderr, "Catalog: 'session' section must exist\n");
        exit(EXIT_FAILURE);
    }
    if (!(queries = json_find(catalog, "queries")))
    {
        fprintf(stderr, "Catalog: 'queries' section must exist\n");
        exit(EXIT_FAILURE);
    }
    json_sort(queries, NULL);
}

static void get_user(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    (void)argv;
    if (argc != 0)
    {
        sqlite3_result_error(context, "user() doesn't take arguments", -1);
        return;
    }
    sqlite3_result_int(context, cookie.user);
}

static void new_token(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    if (argc != 2)
    {
        sqlite3_result_error(context, "new_token() takes 2 arguments", -1);
        return;
    }

    int user = sqlite3_value_int(argv[0]);
    int role = sqlite3_value_int(argv[1]);

    if (!cookie_create(user, role, cookie_str))
    {
        sqlite3_result_error(context, "new_token() failed", -1);
        return;
    }
    sqlite3_result_text(context, strrchr(cookie_str, ':') + 1, -1, SQLITE_STATIC);
}

static void load(const char *path_catalog, const char *path_db)
{
    json_error_t error = {0};

    printf("Loading '%s'\n", path_catalog);
    if (!(catalog = json_parse_file(path_catalog, &error)))
    {
        json_print_error(&error);
        exit(EXIT_FAILURE);
    }
    printf("Loading '%s'\n", path_db);
    if (sqlite3_open(path_db, &db))
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    load_db();

    int status;

    status = sqlite3_create_function(
        db, "user", 0, SQLITE_UTF8, NULL, get_user, NULL, NULL);
    if (status != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    status = sqlite3_create_function(
        db, "new_token", 2, SQLITE_UTF8, NULL, new_token, NULL, NULL);
    if (status != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
}

static void unload(void)
{
    buffer_clean(&buffer);
    json_free(catalog);
    sqlite3_close(db);
}

void writer_load(const char *path_catalog, const char *path_db)
{
    atexit(unload);
    load(path_catalog, path_db);
}

void writer_reload(const char *path_catalog, const char *path_db)
{
    unload();
    load(path_catalog, path_db);
}

static void set_cookie(const json_t *object)
{
    cookie.user = (int)object->child[COOKIE_USER]->number;
    cookie.role = (int)object->child[COOKIE_ROLE]->number;
    cookie.token = object->child[COOKIE_TOKEN]->string;
    cookie_str[0] = '\0';
}

static int verify_cookie(void)
{
    const char *sql = json_string(json_find(session, "verify"));

    if (sql == NULL)
    {
        fprintf(stderr, "Session: A 'verify' string must exist\n");
        exit(EXIT_FAILURE);
    }

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        return 0;
    }

    if ((sqlite3_bind_int(stmt, 1, cookie.user) != SQLITE_OK) ||
        (sqlite3_bind_int(stmt, 2, cookie.role) != SQLITE_OK) ||
        (sqlite3_bind_text(stmt, 3, cookie.token, -1, SQLITE_STATIC) != SQLITE_OK))
    {
        goto error;
    }

    int step, verified = 0;

    while ((step = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        verified = sqlite3_column_int(stmt, 0);
    }
    if (step != SQLITE_DONE)
    {
        goto error;
    }
    sqlite3_finalize(stmt);
    return verified;
error:
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return 0;
}

static const char *get_sql(const json_t *path)
{
    char *key = json_string(json_head(path));

    return json_string(json_search(queries, &(json_t){.key = key}, NULL));
}

static int set_content(sqlite3_stmt *stmt, const json_t *content)
{
    for (unsigned i = 0; i < content->size; i++)
    {
        const json_t *child = content->child[i];
        int status = SQLITE_OK;
        char key[128];
        int index;

        snprintf(key, sizeof key, ":%s", child->key);
        if ((index = sqlite3_bind_parameter_index(stmt, key)) == 0)
        {
            return 0;
        }
        switch (child->type)
        {
            case JSON_STRING:
                status = sqlite3_bind_text(stmt, index, child->string, -1, SQLITE_STATIC);
                break;
            case JSON_INTEGER:
                status = sqlite3_bind_int64(stmt, index, (int64_t)child->number);
                break;
            case JSON_REAL:
                status = sqlite3_bind_double(stmt, index, child->number);
                break;
            case JSON_TRUE:
                status = sqlite3_bind_int(stmt, index, 1);
                break;
            case JSON_FALSE:
                status = sqlite3_bind_int(stmt, index, 0);
                break;
            case JSON_NULL:
                status = sqlite3_bind_null(stmt, index);
                break;
            default:
                return 0;
        }
        if (status != SQLITE_OK)
        {
            return 0;
        }
    }
    return 1;
}

static int set_query(sqlite3_stmt *stmt, const json_t *query)
{
    for (unsigned i = 0; i < query->size; i++)
    {
        const json_t *child = query->child[i];
        char key[128];
        int index;

        snprintf(key, sizeof key, "@%s", child->key);
        if ((index = sqlite3_bind_parameter_index(stmt, key)) == 0)
        {
            return 0;
        }
        if (child->type != JSON_STRING)
        {
            return 0;
        }

        int status = sqlite3_bind_text(stmt, index, child->string, -1, SQLITE_STATIC);

        if (status != SQLITE_OK)
        {
            return 0;
        }
    }
    return 1;
}

static int set_path(sqlite3_stmt *stmt, const json_t *path)
{
    for (unsigned i = 1; i < path->size; i++)
    {
        const json_t *child = path->child[i];
        char key[3];
        int index;

        snprintf(key, sizeof key, "$%u", i % 10);
        if ((index = sqlite3_bind_parameter_index(stmt, key)) == 0)
        {
            return 0;
        }
        if (child->type != JSON_STRING)
        {
            return 0; 
        }

        int status = sqlite3_bind_text(stmt, index, child->string, -1, SQLITE_STATIC);

        if (status != SQLITE_OK)
        {
            return 0;
        }
    }
    return 1;
}

static int db_handle(const json_t *request)
{
    if (cookie.role && !verify_cookie())
    {
        const char *error = "Unauthorized";

        json_write_line(request, stderr);
        fprintf(stderr, "%s\n", error);
        buffer_write(&buffer, error);
        return HTTP_UNAUTHORIZED;
    }

    const json_t *path = json_find(request, "path");
    const json_t *query = json_find(request, "query");
    const json_t *content = json_find(request, "content");
    const char *sql = get_sql(path);

    if (!sql || !path || !query || !content)
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
    if (!set_content(stmt, content) || !set_path(stmt, path) || !set_query(stmt, query))
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        buffer_write(&buffer, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return HTTP_BAD_REQUEST;
    }

    int step, result = HTTP_NO_CONTENT;

    while ((step = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *text = sqlite3_column_text(stmt, 0);

        buffer_write(&buffer, (const char *)text);
    }
    if (step != SQLITE_DONE)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        buffer_write(&buffer, sqlite3_errmsg(db));
        result = HTTP_BAD_REQUEST;
    }
    else if (buffer.length > 0)
    {
        result = HTTP_OK;
    }
    sqlite3_finalize(stmt);
    return result;
}

static void cleanup(json_t *request)
{
    json_delete(json_find(request, "content"));
    request->size = 0;
}

static const buffer_t *process(int header)
{
    char strings[COOKIE_SIZE + 256];
    char headers[COOKIE_SIZE + 384];
    const char *code, *type;

    switch (header)
    {
        case HTTP_OK:
            code = http_ok;
            type = "application/json\r\n";
            break;
        case HTTP_NO_CONTENT:
            code = http_no_content;
            type = "\r\n";
            break;
        case HTTP_UNAUTHORIZED:
            code = http_unauthorized;
            type = "text/plain\r\n";
            break;
        case HTTP_FORBIDDEN:
            code = http_forbidden;
            type = "text/plain\r\n";
            break;
        case HTTP_NOT_FOUND:
            code = http_not_found;
            type = "text/plain\r\n";
            break;
        case HTTP_SERVER_ERROR:
            code = http_server_error;
            type = "text/plain\r\n";
            break;
        default:
            code = http_bad_request;
            type = "text/plain\r\n";
            break;
    }
    if (cookie_str[0] != '\0')
    {
        // Set-Cookie: auth=token; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=31536000
        // Max-Age de un año
        // Con https recordar secure
        snprintf(strings, sizeof strings,
            "%sSet-Cookie: auth=%s; Path=/; HttpOnly; SameSite=Strict; Max-Age=31536000",
            type, cookie_str);
    }
    else
    {
        snprintf(strings, sizeof strings, "%sServer: Unix", type);
    }
    if (header != HTTP_NO_CONTENT)
    {
        snprintf(headers, sizeof headers, code, strings, buffer.length);
    }
    else
    {
        snprintf(headers, sizeof headers, code, strings);
    }
    buffer_insert(&buffer, 0, headers, strlen(headers));
    return buffer.length ? &buffer : NULL;
}

const buffer_t *writer_handle(json_t *request)
{
    int result = schema_validate(request, &buffer);

    set_cookie(json_find(request, "cookie"));
    if (result == HTTP_OK)
    {
        buffer_reset(&buffer);
        result = db_handle(request);
        cleanup(request);
    }
    return process(result);
}

