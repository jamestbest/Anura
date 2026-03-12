#include <gtk/gtk.h>

static bool set_buffer_as_file(const char* file, GtkTextBuffer* buff) {
    char* content;
    gsize length= 0;
    if (g_file_get_contents(file, &content, &length, NULL)) {
        gtk_text_buffer_set_text(buff, content, length);
        g_free(content);
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
}

GtkTextBuffer *buffer;
GtkWidget *view;

static void
activate (GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *scrolled;
    GtkTextIter start, end;
    GtkTextTag *tag;
    GtkCssProvider *provider;
    GdkDisplay *display;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_window_set_child(GTK_WINDOW(window), box);

    view = gtk_text_view_new();
    gtk_widget_set_hexpand(view, TRUE);
    gtk_widget_set_vexpand(view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), false);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    // gtk_text_buffer_set_text(buffer, "Hello, this is some text", -1);
    set_buffer_as_file("/home/james/UoNDocs/ATOMIC/Compiler.c", buffer);


    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        provider,
        "textview {"
        "  font-size: 15px;"
        "  font-family: serif;"
        "  color: white;"
        "}",
        -1
    );

    display = gtk_widget_get_display(view);
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);

    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 30);

    scrolled = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), view);

    gtk_box_append(GTK_BOX(box), scrolled);

    GtkEventController* controller= gtk_event_controller_key_new();
    g_signal_connect_object(controller, "key-pressed", G_CALLBACK(event_key_pressed_cb), view, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(window, controller);

    gtk_window_present(GTK_WINDOW(window));
}

void goto_line() {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(buffer, &iter, 85);
    GtkTextMark* mark= gtk_text_buffer_create_mark(buffer, "main", &iter, true);

    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(view), mark, 0, true, 0, 0);
}


int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
