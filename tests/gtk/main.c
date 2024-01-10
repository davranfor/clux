/*!
 *  \brief     clux - Ligtweight json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/*
For this example you may need to install gtk4 development libraries

Install gtk4 on Debian based:
sudo apt install libgtk-4-dev

Install gtk4 on macos:
brew install pkg-config gtk4

And then run:
make
*/

#include <locale.h>
#include <gtk/gtk.h>
#include <clux/json.h>
#include "utils.h"

#define DEFAULT_TITLE "Undefined"
#define DEFAULT_WIDTH 1600
#define DEFAULT_HEIGHT 900

static const char *app_title(const json *app_data)
{
    const char *title = json_string(json_find(app_data, "title"));

    return title ? title : DEFAULT_TITLE;
}

static int app_width(const json *app_data)
{
    int width = (int)json_integer(json_find(app_data, "width"));

    return width > 0 ? width : DEFAULT_WIDTH;
}

static int app_height(const json *app_data)
{
    int height = (int)json_integer(json_find(app_data, "height"));

    return height > 0 ? height : DEFAULT_HEIGHT;
}

static int app_prefer_dark_theme(const json *app_data)
{
    return json_is_true(json_find(app_data, "prefer-dark-theme"));
}

static void app_print_data(GtkTextBuffer *text_buffer, const json *app_data)
{
    char *text = json_indent(app_data, 2);

    if (text != NULL)
    {
        gtk_text_buffer_set_text(text_buffer, text, -1);
        free(text);
    }
}

static void activate(GtkApplication *app, gpointer user_data)
{
    const json *app_data = user_data;

    if (app_prefer_dark_theme(app_data))
    {
        g_object_set(gtk_settings_get_default(),
            "gtk-application-prefer-dark-theme", TRUE, NULL);
    }

    GtkWidget *window = gtk_application_window_new(app);

    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);
    gtk_window_set_title(GTK_WINDOW(window), app_title(app_data));
    gtk_window_set_default_size(GTK_WINDOW(window),
        app_width(app_data), app_height(app_data));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_window_destroy), NULL);

    GtkWidget *header_bar = gtk_header_bar_new();

    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();

    gtk_window_set_child(GTK_WINDOW(window), scrolled_window);

    GtkWidget *text_view = gtk_text_view_new();

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), text_view);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), true);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), false);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 5);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 5);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_view), 5);

    GtkTextBuffer *text_buffer = gtk_text_buffer_new(NULL);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), text_buffer);

    app_print_data(text_buffer, app_data);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[])
{
    gtk_disable_setlocale();
    setlocale(LC_CTYPE, "");

    json *app_data = parse_file("app.json");
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), app_data);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    json_free(app_data);
    return status;
}

