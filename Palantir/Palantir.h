//
// Created by James Coward on 1/29/26.
//

#ifndef ANURA_PALANTIR_GUI_H
#define ANURA_PALANTIR_GUI_H

#include <gtk/gtk.h>

void* gui_thread_create(void* data);
gboolean guiup_main_file(gpointer main_filepath);
gboolean terminal_log(gpointer data);
gboolean terminal_err(gpointer data);

#endif //ANURA_PALANTIR_GUI_H