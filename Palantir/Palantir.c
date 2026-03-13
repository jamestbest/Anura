#include "Palantir/Palantir.h"

#include <gtk/gtk.h>

#include "main.h"

int* stdio_pipe;

static bool set_buffer_as_file(const char* file, GtkTextBuffer* buff) {
    char* content;
    gsize length= 0;
    printf("Trying to set buffer as file");
    gtk_text_buffer_set_text(buff, "", -1);

    if (g_file_get_contents(file, &content, &length, NULL)) {

        GtkTextIter end_iter;
        const char** lines= g_strsplit(content, "\n", -1);
        g_free(content);

        for (int i = 0; lines[i] != NULL; ++i) {
            char* num= malloc(10);
            snprintf(num, 10, "%4d  ", i);
            gtk_text_buffer_get_end_iter(buff, &end_iter);
            gtk_text_buffer_insert(buff, &end_iter, num, -1);
            gtk_text_buffer_insert(buff, &end_iter, lines[i], -1);
            gtk_text_buffer_insert(buff, &end_iter, "\n", -1);
            free(num);
        }
        return true;
    }
    return false;
}

void goto_line();

static gboolean event_key_pressed_cb(
    GtkWidget             *drawing_area,
    guint                  keyval,
    guint                  keycode,
    GdkModifierType        state,
    GtkEventControllerKey *event_controller
) {
    printf("Got event\n");
    goto_line();

    return true;
}

GtkTextBuffer* code_buffer;

GtkWidget* code_view;
GtkWidget* code_scroller;

GtkWidget* terminal_view;
GtkWidget* output_view;

GtkApplication* app;

gboolean guiup_main_file(gpointer main_filepath) {
    if (main_filepath == NULL) {
        printf("Failed to find main file\n");
        return false;
    }

    printf("Got the main file as %s\n", (char*)main_filepath);
    set_buffer_as_file(main_filepath, code_buffer);

    return false;
}

static void open_file_response(GtkDialog *dialog, int response){
    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser* chooser= GTK_FILE_CHOOSER(dialog);
        GFile* file= gtk_file_chooser_get_file(chooser);

        const char* path= g_file_get_path(file);
        g_print("Selected file: %s\n", path);
        open_program(path);
        g_object_unref(file);
    }

    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void open_file_action(GSimpleAction* action, GVariant* param, gpointer data) {
    GtkWindow* window= GTK_WINDOW(data);
    GtkWidget* dialog= gtk_file_chooser_dialog_new ("Open File",
        window,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        "_Open",
        GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect(
        dialog, "response",
        G_CALLBACK (open_file_response),
        NULL
    );
}

static GtkWidget* make_menu_bar() {
    GMenu* menu= g_menu_new();
    GMenu* file= g_menu_new();

    g_menu_append(file, "open", "app.open");
    g_menu_append(file, "quit", "app.quit");
    g_menu_append_submenu(menu, "file", G_MENU_MODEL(file));

    GtkWidget* menu_bar= gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menu));

    return menu_bar;
}

static GtkWidget* create_terminal_scroller() {
    GtkWidget* terminal_scroller= gtk_scrolled_window_new();
    gtk_widget_set_hexpand(terminal_scroller, TRUE);
    gtk_widget_set_halign(terminal_scroller, GTK_ALIGN_FILL);
    gtk_widget_set_size_request(terminal_scroller, -1, 200);

    terminal_view= gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(terminal_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(terminal_view), FALSE);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(terminal_scroller), terminal_view);

    return terminal_scroller;
}

static GtkWidget* create_output_scroller() {
    GtkWidget* output_scroller= gtk_scrolled_window_new();
    gtk_widget_set_hexpand(output_scroller, TRUE);
    gtk_widget_set_halign(output_scroller, GTK_ALIGN_FILL);
    gtk_widget_set_size_request(output_scroller, -1, 200);

    output_view= gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(output_view), FALSE);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(output_scroller), output_view);

    return output_scroller;
}

gboolean terminal_err(gpointer data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(terminal_view));

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);

    gtk_text_buffer_insert(buff, &end, "ERR: ", -1);
    gtk_text_buffer_insert(buff, &end, data, -1);
    // free(data);

    gtk_text_buffer_insert(buff, &end, "\n", -1);

    return false;
}

gboolean terminal_log(gpointer data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(terminal_view));

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);

    gtk_text_buffer_insert(buff, &end, data, -1);
    // free(data);

    const size_t len= strlen(data);
    if (len == 0 || ((char*)data)[len - 1] != '\n')
        gtk_text_buffer_insert(buff, &end, "\n", -1);

    return false;
}

gboolean output_log(gpointer data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);

    gtk_text_buffer_insert(buff, &end, data, -1);
    // free(data);

    const size_t len= strlen(data);
    if (len == 0 || ((char*)data)[len - 1] != '\n')
        gtk_text_buffer_insert(buff, &end, "\n", -1);

    return false;
}

//todo
static gboolean read_target_output_cb(GIOChannel *source, GIOCondition condition, gpointer user_data) {
    gchar buff[1024];
    gsize bytes_read;
    GError* error= NULL;

    printf("Truing to get output\n");

    const GIOStatus status= g_io_channel_read_chars(source, buff, sizeof(buff) - 1, &bytes_read, &error);
    if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
        buff[bytes_read]= '\0';
        output_log(buff);
    }

    if (status == G_IO_STATUS_ERROR) {
        g_error("Error reading child stdout: %s", error->message);
        g_clear_error(&error);
    }

    return TRUE;
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window= gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Anura: GUI Debugger");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);

    GtkWidget* box= gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget* top_ui= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(box), top_ui);

    GtkWidget* code_box= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget* menu_bar= make_menu_bar();
    GtkWidget* run_button= gtk_button_new_with_label("run");

    gtk_box_append(GTK_BOX(top_ui), menu_bar);
    gtk_box_append(GTK_BOX(top_ui), run_button);
    gtk_widget_set_halign(run_button, GTK_ALIGN_END);
    gtk_widget_set_hexpand(run_button, TRUE);

    code_view= gtk_text_view_new();
    gtk_widget_set_hexpand(code_view, TRUE);
    gtk_widget_set_vexpand(code_view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(code_view), false);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(code_view), TRUE);

    code_buffer= gtk_text_view_get_buffer(GTK_TEXT_VIEW(code_view));
    // gtk_text_buffer_set_text(buffer, "Hello, this is some text", -1);
    set_buffer_as_file("/home/jamestbest/test.s", code_buffer);

    GtkCssProvider* provider= gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        provider,
        "textview {"
        "  font-size: 15px;"
        "  font-family: monospace;"
        "  color: black;"
        "}",
        -1
    );

    GdkDisplay* display= gtk_widget_get_display(code_view);
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);

    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(code_view), 30);

    GtkTextTag* tag= gtk_text_buffer_create_tag(
        code_buffer,
        "blue_foreground",
        "foreground", "blue",
        NULL
    );

    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_offset(code_buffer, &start, 7);
    gtk_text_buffer_get_iter_at_offset(code_buffer, &end, 12);
    gtk_text_buffer_apply_tag(code_buffer, tag, &start, &end);

    code_scroller= gtk_scrolled_window_new();
    gtk_widget_set_hexpand(code_scroller, TRUE);
    gtk_widget_set_vexpand(code_scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(code_scroller), code_view);

    gtk_box_append(GTK_BOX(code_box), code_scroller);

    GtkWidget* terminal_scroller= create_terminal_scroller();
    GtkWidget* output_scroller= create_output_scroller();
    GtkWidget* terminals= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    gtk_box_append(GTK_BOX(terminals), terminal_scroller);
    gtk_box_append(GTK_BOX(terminals), output_scroller);

    gtk_box_append(GTK_BOX(box), code_box);
    gtk_box_append(GTK_BOX(box), terminals);

    GtkEventController* controller= gtk_event_controller_key_new();
    g_signal_connect_object(controller, "key-pressed", G_CALLBACK(event_key_pressed_cb), code_view, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(window, controller);

    terminal_log("Hello world!");

    // GIOChannel* channel = g_io_channel_unix_new(stdio_pipe[0]);
    // g_io_add_watch(channel, G_IO_IN | G_IO_HUP, read_target_output_cb, NULL);

    gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_present(GTK_WINDOW(window));
}

void goto_line() {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(code_buffer, &iter, 85);
    gtk_text_iter_forward_to_line_end(&iter);

    GtkTextMark* mark= gtk_text_buffer_create_mark(code_buffer, "main", &iter, true);

    gtk_text_buffer_place_cursor(code_buffer, &iter);

    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(code_view), mark, 0, true, 0, 0);
}

static void quit_action(GSimpleAction *action, GVariant *parameter, gpointer app) {
    g_application_quit(G_APPLICATION(app));
}

static void create_quit_mapping() {
    GSimpleAction *quit = g_simple_action_new("quit", NULL);
    g_signal_connect(quit, "activate", G_CALLBACK(quit_action), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit));
}

static void create_open_mapping() {
    GSimpleAction *open_action = g_simple_action_new("open", NULL);
    g_signal_connect(open_action, "activate", G_CALLBACK(open_file_action), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(open_action));
}

static void create_mappings() {
    create_quit_mapping();
    create_open_mapping();
}

int create_gui() {
    app= gtk_application_new ("org.anura.main", G_APPLICATION_FLAGS_NONE);
    create_mappings();

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    const int status= g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return status;
}


void* gui_thread_create(void* data) {
    printf("The pipe data is %d\n", *(int*)data);
    stdio_pipe= (int*)data;

    create_gui();

    return NULL;
}

// int main(const int argc, char **argv) {
//     if (argc > 1) perror("Command line arguments are currently ignored");
//
//     create_gui();
// }
