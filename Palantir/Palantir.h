//
// Created by James Coward on 1/29/26.
//

#ifndef ANURA_PALANTIR_GUI_H
#define ANURA_PALANTIR_GUI_H

#include <gtk/gtk.h>
#include "break_on_cause.h"

int create_gui(int pipe[2]);
gboolean guiup_main_file(gpointer main_filepath);
gboolean terminal_log(gpointer data);
gboolean terminal_newline(gpointer data);
gboolean terminal_err(gpointer data);
gboolean update_target_data(gpointer data);
gboolean update_user_breakpoints(gpointer data);
gboolean hit_line(gpointer data);
gboolean hit_addr(gpointer data);
gboolean update_breakpoint_displays(gpointer data);
gboolean update_breakpoint_memory(gpointer data);

gboolean display_break_cause_tree(gpointer root);
gboolean display_break_save_tree(gpointer root);
gboolean display_all_registers(gpointer data);
gboolean display_stack_trace(gpointer data);

#endif //ANURA_PALANTIR_GUI_H