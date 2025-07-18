/*!
 *  \brief     C library for unixes
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <clux/clib.h>
#include <clux/json.h>
#include <clux/json_private.h>
#include <sqlite3.h>
#include "header.h"
#include "loader.h"
#include "schema.h"
#include "cookie.h"
#include "writer.h"

static sqlite3 *db;
static json_t *catalog;
static json_t *session;
static json_t *queries;

static cookie_t cookie;
static char cookie_str[COOKIE_SIZE];
static int workplace;

static buffer_t buffer;

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

static void db_session_id(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    (void)argv;
    if (argc != 0)
    {
        sqlite3_result_error(context, "session_id() doesn't take arguments", -1);
        return;
    }
    sqlite3_result_int(context, cookie.user);
}

static void db_workplace_id(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    (void)argv;
    if (argc != 0)
    {
        sqlite3_result_error(context, "workplace_id() doesn't take arguments", -1);
        return;
    }
    sqlite3_result_int(context, workplace);
}

static void db_role(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    (void)argv;
    if (argc != 0)
    {
        sqlite3_result_error(context, "role() doesn't take arguments", -1);
        return;
    }
    sqlite3_result_int(context, cookie.role);
}

static void db_new_token(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    if (argc != 3)
    {
        sqlite3_result_error(context, "new_token() takes 3 arguments", -1);
        return;
    }

    int user = sqlite3_value_int(argv[0]);
    int role = sqlite3_value_int(argv[1]);
    const unsigned char *token = sqlite3_value_text(argv[2]);

    if (!cookie_create(user, role, (const char *)token, cookie_str))
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
        db, "session_id", 0, SQLITE_UTF8, NULL, db_session_id, NULL, NULL);
    if (status != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    status = sqlite3_create_function(
        db, "workplace_id", 0, SQLITE_UTF8, NULL, db_workplace_id, NULL, NULL);
    if (status != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    status = sqlite3_create_function(
        db, "role", 0, SQLITE_UTF8, NULL, db_role, NULL, NULL);
    if (status != SQLITE_OK)
    {
        fprintf(stderr, "%s\n", sqlite3_errmsg(db));
        exit(EXIT_FAILURE);
    }
    status = sqlite3_create_function(
        db, "new_token", 3, SQLITE_UTF8, NULL, db_new_token, NULL, NULL);
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
        workplace = sqlite3_column_int(stmt, 0);
        verified = 1;
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

        int status = sqlite3_bind_text(stmt, index, child->string, -1, SQLITE_STATIC);

        if (status != SQLITE_OK)
        {
            return 0;
        }
    }
    return 1;
}

static int action_handle(const json_t *path)
{
    const char *action = json_string(json_head(path));

    if (action != NULL)
    {
        if (!strcmp(action, "reload"))
        {
            loader_reload();
            return 1;
        }
        if (!strcmp(action, "stop"))
        {
            exit(EXIT_SUCCESS);
        }
    }
    return 0;
}

static int db_handle(const json_t *request)
{
    if (cookie.role && !verify_cookie())
    {
        const char *msg = "Unauthorized";

        fprintf(stderr, "%s\n", msg);
        buffer_write(&buffer, msg);
        return HTTP_UNAUTHORIZED;
    }

    const json_t *path = json_find(request, "path");
    const json_t *query = json_find(request, "query");
    const json_t *content = json_find(request, "content");
    const char *sql = get_sql(path);

    if (!path || !query || !content || !sql)
    {
        if (action_handle(path))
        {
            return HTTP_NO_CONTENT;
        }

        const char *msg = "Malformed query";

        fprintf(stderr, "%s\n", msg);
        buffer_write(&buffer, msg);
        return HTTP_SERVER_ERROR;
    }

    int writting;
    char *error;

    // Methods POST, PUT, PATCH and DELETE starts a transaction, GET doesn't
    if ((writting = strcmp(request->key, "GET")) &&
        (sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &error) != SQLITE_OK))
    {
        fprintf(stderr, "%s\n", error);
        sqlite3_free(error);
        exit(EXIT_FAILURE);
    }

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        goto bad_request;
    }
    if (!set_content(stmt, content) || !set_path(stmt, path) || !set_query(stmt, query))
    {
        goto bad_request;
    }

    int step, result = HTTP_NO_CONTENT;

    while ((step = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *text = sqlite3_column_text(stmt, 0);

        buffer_write(&buffer, text ? (const char *)text : "null");
    }
    if (step != SQLITE_DONE)
    {
        goto bad_request;
    }
    else if (buffer.length > 0)
    {
        result = HTTP_OK;
    }
    sqlite3_finalize(stmt);
    if (writting && (sqlite3_exec(db, "COMMIT;", NULL, NULL, &error) != SQLITE_OK))
    {
        fprintf(stderr, "%s\n", error);
        sqlite3_free(error);
        exit(EXIT_FAILURE);
    }
    return result;
bad_request:
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    buffer_write(&buffer, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    if (writting && (sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &error) != SQLITE_OK))
    {
        fprintf(stderr, "%s\n", error);
        sqlite3_free(error);
        exit(EXIT_FAILURE);
    }
    return HTTP_BAD_REQUEST;
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
#ifdef ALLOW_INSECURE_TOKEN
        /**
         * For testing purposes where you can not provide an SSL connection:
         * Some browsers (i.e. Safari) doesn't send a Secure token on non-https connections even
         * for testing with localhost (https requires 'Secure;')
         * You can set an environment variable on .zshrc or .bashrc:
         * export CLUX_ALLOW_INSECURE_TOKEN=1
         * Then, inside the Makefile, there is a rule to add a preprocessor flag:
         * ifdef CLUX_ALLOW_INSECURE_TOKEN
         * CFLAGS += -DALLOW_INSECURE_TOKEN
         * endif
         * Depending on this flag, the 'Secure;' flag is sent or not to the client.
         * Max-Age = 1 year
         */
        snprintf(strings, sizeof strings,
            "%sSet-Cookie: auth=%s; Path=/; HttpOnly; SameSite=Strict; Max-Age=31536000",
            type, cookie_str);
#else
        snprintf(strings, sizeof strings,
            "%sSet-Cookie: auth=%s; Path=/; Secure; HttpOnly; SameSite=Strict; Max-Age=31536000",
            type, cookie_str);
#endif
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
    // Debug
    if (buffer.length)
    {
        printf("---- RESPONSE ----\n%s\n", buffer.text);
    }
    return buffer.length ? &buffer : NULL;
}

const buffer_t *writer_handle(json_t *request)
{
    set_cookie(json_find(request, "cookie"));

    int result = schema_validate(request, &buffer);

    if (result == HTTP_OK)
    {
        buffer_reset(&buffer);
        result = db_handle(request);
        cleanup(request);
    }
    return process(result);
}

