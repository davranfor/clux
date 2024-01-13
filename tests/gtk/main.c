/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

/*
For this example you may need to install gtk4 development libraries

On Debian:
sudo apt install libgtk-4-dev

On macos:
brew install pkg-config gtk4

And then run:
make && ./demo
*/

#include <locale.h>
#include <gtk/gtk.h>
#include <clux/json.h>
#include "utils.h"

#define APP_DEFAULT_TITLE "Undefined"
#define APP_DEFAULT_VERSION "Unknown"
#define APP_DEFAULT_WIDTH 1024
#define APP_DEFAULT_HEIGHT 768

static const json *app_data;

static const char *app_get_title(void)
{
    const char *title = json_string(json_find(app_data, "title"));

    return title != NULL ? title : APP_DEFAULT_TITLE;
}

static const char *app_get_version(void)
{
    const char *version = json_string(json_find(app_data, "version"));

    return version != NULL ? version : APP_DEFAULT_VERSION;
}

static int app_get_width(void)
{
    int width = (int)json_integer(json_find(app_data, "width"));

    return width > 0 ? width : APP_DEFAULT_WIDTH;
}

static int app_get_height(void)
{
    int height = (int)json_integer(json_find(app_data, "height"));

    return height > 0 ? height : APP_DEFAULT_HEIGHT;
}

static int app_get_prefer_dark_theme(void)
{
    return json_is_true(json_find(app_data, "prefer-dark-theme"));
}

static void app_print_data(GtkTextBuffer *text_buffer)
{
    char *text = json_indent(app_data, 2);

    if (text != NULL)
    {
        gtk_text_buffer_set_text(text_buffer, text, -1);
        free(text);
    }
}

static void app_about(GSimpleAction *action, GVariant *parameter, gpointer app)
{
    (void)action;
    (void)parameter;
    (void)app;

    const char *title = app_get_title();
    const char *version = app_get_version();

    char detail[32];

    snprintf(detail, sizeof detail, "Version: %s", version);

    GtkAlertDialog *alert_dialog = gtk_alert_dialog_new("\n%s", title);

    gtk_alert_dialog_set_detail(alert_dialog, detail);
    gtk_alert_dialog_set_modal(alert_dialog, true);
    gtk_alert_dialog_show(alert_dialog, gtk_application_get_active_window(app));
    g_object_unref(alert_dialog);
}

static void app_quit(GSimpleAction *action, GVariant *parameter, gpointer app)
{
    (void)action;
    (void)parameter;
    g_application_quit(G_APPLICATION(app));
}

static GActionEntry app_entries[] =
{
    {"about", app_about, NULL, NULL, NULL, {0}},
    {"quit", app_quit, NULL, NULL, NULL, {0}}
};

static void app_create_menu_bar(GtkApplication *app, GtkWidget *window)
{
    GMenu *menu = g_menu_new();

    g_menu_append(menu, "About", "app.about");
    g_menu_append(menu, "Quit", "app.quit");

    GtkWidget *menu_button = gtk_menu_button_new();

    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));

    g_object_unref(menu);

    const char *quit_accels[2] = {"<Ctrl>Q", NULL};

    g_action_map_add_action_entries(
        G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);
    gtk_application_set_accels_for_action(app, "app.quit", quit_accels);

    GtkWidget *header_bar = gtk_header_bar_new();

    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), menu_button);
    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), true);
}

static GtkWidget *app_create_text_view(GtkWidget *window)
{
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

    return text_view;
}

static void app_activate(GtkApplication *app, gpointer user_data)
{
    app_data = user_data;

    g_object_set(gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", app_get_prefer_dark_theme(), NULL);

    GtkWidget *window = gtk_application_window_new(app);

    gtk_window_set_title(GTK_WINDOW(window), app_get_title());
    gtk_window_set_default_size(GTK_WINDOW(window),
        app_get_width(), app_get_height());
    gtk_window_set_resizable(GTK_WINDOW(window), false);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_window_destroy), NULL);

    app_create_menu_bar(app, window);

    GtkWidget *text_view = app_create_text_view(window);
    GtkTextBuffer *text_buffer = gtk_text_buffer_new(NULL);

    gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), text_buffer);

    app_print_data(text_buffer);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[])
{
    gtk_disable_setlocale();
    setlocale(LC_CTYPE, "");

    json *data = parse_file("app.json");
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), data);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    json_free(data);
    return status;
}

