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

static void
activate (GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *view;
    GtkWidget *scrolled;
    GtkTextBuffer *buffer;
    GtkTextIter start, end;
    GtkTextTag *tag;
    GtkCssProvider *provider;
    GdkDisplay *display;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_window_set_child(GTK_WINDOW(window), box);

    view = gtk_text_view_new();
    gtk_widget_set_hexpand(view, TRUE);
    gtk_widget_set_vexpand(view, TRUE);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    // gtk_text_buffer_set_text(buffer, "Hello, this is some text", -1);
    set_buffer_as_file("/home/jamestbest/test.s", buffer);

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(
        provider,
        "textview {"
        "  font-size: 15px;"
        "  font-family: serif;"
        "  color: green;"
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

    tag = gtk_text_buffer_create_tag(
        buffer,
        "blue_foreground",
        "foreground", "blue",
        NULL
    );

    gtk_text_buffer_get_iter_at_offset(buffer, &start, 7);
    gtk_text_buffer_get_iter_at_offset(buffer, &end, 12);
    gtk_text_buffer_apply_tag(buffer, tag, &start, &end);

    scrolled = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), view);

    gtk_box_append(GTK_BOX(box), scrolled);

    gtk_window_present(GTK_WINDOW(window));
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
