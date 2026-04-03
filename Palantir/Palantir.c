#include "Palantir/Palantir.h"

#include <gtk/gtk.h>

#include "break_on_cause.h"
#include "Helper_Math.h"
#include "main.h"
#include "MtDoom.h"
#include "Target.h"
#include "output/default.h"

void goto_line(int line);
void fill_disassembly_view();

int* stdio_pipe;

GtkTextBuffer* code_buffer;

GtkWidget* code_view;
GtkWidget* user_bps;
GtkWidget* code_scroller;

GtkWidget* disassembly_view;
GtkWidget* all_bps;
GtkWidget* disassembly_scroller;

GtkWidget* memory_view;

GtkWidget* terminal_view;
GtkWidget* output_view;

GtkApplication* app;

GtkTextTag* user_bp_hit_highlight= NULL;
size_t user_bp_hit_highlight_line= -1;

GtkTextTag* all_bp_hit_highlight= NULL;
size_t all_bp_hit_highlight_line= -1;

GHashTable* address_to_mark;
typedef struct LocInfo {
    GtkTextMark* mark;
    uintptr_t address;
    uint32_t bytes;
} LocInfo;

LocInfo* alloc_loc_info(GtkTextMark* mark, uintptr_t addr, uint32_t bytes) {
    LocInfo* info= malloc(sizeof(LocInfo));
    *info= (LocInfo) {
        .mark= mark,
        .address= addr,
        .bytes= bytes,
    };

    return info;
}

void highlight_diss_addr(uint32_t addr) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(disassembly_view));
    if (!all_bp_hit_highlight) all_bp_hit_highlight= gtk_text_buffer_create_tag(
        buff,
        "all-bp-hit-highlight",
        "paragraph-background", "#ee3b2b",
        NULL
    );

    if (all_bp_hit_highlight_line != -1) {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buff, &start);
        gtk_text_buffer_get_end_iter(buff, &end);
        gtk_text_buffer_remove_tag(buff, all_bp_hit_highlight, &start, &end);
        all_bp_hit_highlight_line= -1;
    }

    LocInfo* info= g_hash_table_lookup(address_to_mark, GINT_TO_POINTER(addr));
    if (!info) {
        show_err("Unable to find line associated with address %#x", addr);
        return;
    }


    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_mark(buff, &start, info->mark);

    end= start;
    gtk_text_iter_forward_to_line_end(&end);
    gtk_text_buffer_apply_tag(buff, all_bp_hit_highlight, &start, &end);
    int line= gtk_text_iter_get_line(&start);

    all_bp_hit_highlight_line= line;
}

void highlight_code_line(uint32_t line) {
    if (!user_bp_hit_highlight) user_bp_hit_highlight= gtk_text_buffer_create_tag(
        code_buffer,
        "bp-hit-highlight",
        "paragraph-background", "#ee3b2b",
        NULL
    );

    if (user_bp_hit_highlight_line != -1) {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(code_buffer, &start);
        gtk_text_buffer_get_end_iter(code_buffer, &end);
        gtk_text_buffer_remove_tag(code_buffer, user_bp_hit_highlight, &start, &end);
        user_bp_hit_highlight_line= -1;
    }

    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_line(code_buffer, &start, line - 1);
    end= start;
    gtk_text_iter_forward_to_line_end(&end);

    gtk_text_buffer_apply_tag(code_buffer, user_bp_hit_highlight, &start, &end);

    user_bp_hit_highlight_line= line - 1;
}

G_DECLARE_FINAL_TYPE(TreeNodeObject, tree_node_object, TREE, NODE_OBJECT, GObject)
struct _TreeNodeObject {
    GObject parent_instance;
    TreeNode* node;
};
G_DEFINE_TYPE(TreeNodeObject, tree_node_object, G_TYPE_OBJECT)

static void tree_node_object_class_init(TreeNodeObjectClass* class) {}
static void tree_node_object_init(TreeNodeObject* self) {}
TreeNodeObject* tree_node_object_new(TreeNode* node) {
    TreeNodeObject* obj= g_object_new(tree_node_object_get_type(), NULL);
    obj->node = node;
    return obj;
}

static GListModel*
create_child_model(gpointer item, gpointer user_data)
{
    TreeNodeObject *obj = item;
    TreeNode* node=  obj->node;

    if (node->links.pos == 0)
        return NULL;

    GListStore* store= g_list_store_new(tree_node_object_get_type());

    for (int i = 0; i < node->links.pos; ++i) {
        TreeNode* child= Node_vec_get_unsafe(&node->links, i);
        g_list_store_append(store, tree_node_object_new(child));
    }

    return G_LIST_MODEL(store);
}

static void setup(GtkListItemFactory *factory,
                  GtkListItem *list_item,
                  gpointer data)
{
    GtkWidget *expander = gtk_tree_expander_new();
    GtkWidget *label = gtk_label_new(NULL);

    gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
    gtk_list_item_set_child(list_item, expander);
}

static void bind(GtkListItemFactory *factory,
                 GtkListItem *list_item,
                 gpointer data)
{
    GtkTreeListRow *row = gtk_list_item_get_item(list_item);
    TreeNodeObject *obj = gtk_tree_list_row_get_item(row);

    GtkWidget *expander = gtk_list_item_get_child(list_item);
    GtkWidget *label =
        gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));

    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander), row);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "CFA: %p instance of %s", (void*)obj->node->frame.cfa, obj->node->frame.subprog_name);

    gtk_label_set_text(GTK_LABEL(label), buffer);
}

static void break_cause_detail_change(GtkSelectionModel* selection, guint pos, guint n, gpointer data) {
    GtkWidget* cont= GTK_WIDGET(data);

    GtkWidget* existing;
    while (existing= gtk_widget_get_first_child(cont), existing != NULL) {
        gtk_box_remove(GTK_BOX(cont), existing);
    }

    GtkTreeListRow* selected_row= gtk_single_selection_get_selected_item(GTK_SINGLE_SELECTION(selection));
    if (!selected_row) return;
    TreeNodeObject* selected= gtk_tree_list_row_get_item(selected_row);
    if (!selected) return;

    TreeNode* node= selected->node;


    GtkWidget *list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    gtk_box_append(GTK_BOX(cont), list_box);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);

    char *cfa_str = g_strdup_printf("CFA: %p - %p", node->frame.cfa, node->frame.end_stack_pointer);
    gtk_list_box_append(GTK_LIST_BOX(list_box), gtk_label_new(cfa_str));
    g_free(cfa_str);

    GtkWidget *expander = gtk_expander_new("Markers List");
    GtkWidget *marker_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(marker_list_box, 20);

    if (node->markers.arr) {
        for (size_t i = 0; i < node->markers.pos; i++) {
            const Marker* marker= Marker_vec_get_unsafe(&node->markers, i);
            if (!marker) continue;
            char* str= g_strdup_printf("Marker @ %#lx prev: %#p", marker->pc, marker->prev);
            gtk_box_append(GTK_BOX(marker_list_box), gtk_label_new(str));
            g_free(str);
        }
    }

    gtk_expander_set_child(GTK_EXPANDER(expander), marker_list_box);
    gtk_list_box_append(GTK_LIST_BOX(list_box), expander);
}

gboolean display_break_cause_tree(gpointer root) {
    GtkWidget* window= gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Break on cause result");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920/2, 1080/2);

    GListStore* root_model= g_list_store_new(tree_node_object_get_type());
    g_list_store_append(root_model, tree_node_object_new(root));

    GtkTreeListModel* tree_list= gtk_tree_list_model_new(
        G_LIST_MODEL(root_model),
        FALSE,
        FALSE,
        create_child_model,
        NULL,
        NULL
    );

    GtkSelectionModel *selection =
    GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(tree_list))
    );

    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind), NULL);

    GtkWidget *view = gtk_list_view_new(selection, factory);

    GtkWidget* main_box= gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    GtkWidget* scroller= gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), view);
    gtk_paned_set_start_child(GTK_PANED(main_box), scroller);
    gtk_paned_set_resize_start_child(GTK_PANED(main_box), TRUE);

    GtkWidget* detail_scroller= gtk_scrolled_window_new();
    GtkWidget* details_view= gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(detail_scroller), details_view);
    gtk_paned_set_end_child(GTK_PANED(main_box), detail_scroller);

    g_signal_connect(selection, "selection-changed", G_CALLBACK(break_cause_detail_change), details_view);

    gtk_window_present(GTK_WINDOW(window));

    return false;
}

typedef struct {
    GtkTextBuffer* mem_buff;
    GtkTextBuffer* diss_buff;
} MemoryViewBuffers;

static void memory_view_add(gpointer key, gpointer value, gpointer data) {
    const LocInfo* info= value;
    const uintptr_t addr= GPOINTER_TO_SIZE(key);

    MemoryViewBuffers* buffers= data;

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffers->diss_buff, &iter, info->mark);
    const int line= gtk_text_iter_get_line(&iter);

    Data d_read= target.target_get_data_virtual(addr, info->bytes);
    const int buff_size= 3 * info->bytes + 1;
    char* buff= malloc(buff_size);
    for (int i = 0; i < info->bytes; ++i) {
        snprintf(&buff[i * 3], 4, "%.2x ", d_read.raw_data[i]);
    }

    gtk_text_buffer_get_iter_at_line(buffers->mem_buff, &iter, line);
    gtk_text_buffer_insert(buffers->mem_buff, &iter, buff, -1);
}

static void fill_memory_view() {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(memory_view));
    gtk_text_buffer_set_text(buff, "", -1);

    GtkTextBuffer* diss_buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(disassembly_view));
    const int lines= gtk_text_buffer_get_line_count(diss_buff);

    MemoryViewBuffers buffers= {
        .diss_buff= diss_buff,
        .mem_buff= buff
    };

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);
    for (int i = 0; i < lines; ++i) {
        gtk_text_buffer_insert(buff, &end, "\n", -1);
    }

    g_hash_table_foreach(address_to_mark, memory_view_add, &buffers);
}

static bool set_buffer_as_file(const char* file, GtkTextBuffer* buff) {
    char* content;
    gsize length= 0;
    printf("Trying to set buffer as file");
    gtk_text_buffer_set_text(buff, "", -1);

    if (g_file_get_contents(file, &content, &length, NULL)) {

        GtkTextIter end_iter;
        char** lines= g_strsplit(content, "\n", -1);
        g_free(content);

        for (int i = 0; lines[i] != NULL; ++i) {
            char* num= malloc(10);
            snprintf(num, 10, "%4d  ", i + 1);
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

static gboolean event_key_pressed_cb(
    GtkWidget             *drawing_area,
    guint                  keyval,
    guint                  keycode,
    GdkModifierType        state,
    GtkEventControllerKey *event_controller
) {
    printf("Got event\n");
    goto_line(88);

    return true;
}

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

    gtk_text_buffer_insert(buff, &end, "\n", -1);

    return false;
}

gboolean terminal_newline(gpointer data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(terminal_view));

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);

    gtk_text_buffer_insert(buff, &end, "\n", -1);

    return false;
}

gboolean terminal_log(gpointer data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(terminal_view));

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);

    gtk_text_buffer_insert(buff, &end, data, -1);

    free(data);

    gtk_text_view_scroll_to_iter(
        GTK_TEXT_VIEW(terminal_view),
        &end,
        0.0,
        FALSE,
        0,
        0
    );

    return false;
}

gboolean output_log(gpointer data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(output_view));

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);

    gtk_text_buffer_insert(buff, &end, data, -1);

    const size_t len= strlen(data);
    if (len == 0 || ((char*)data)[len - 1] != '\n')
        gtk_text_buffer_insert(buff, &end, "\n", -1);

    return false;
}

//todo remove buff overflow
static gboolean read_target_output_pipe(GIOChannel *source, GIOCondition condition, gpointer user_data) {
    gchar buff[1024];
    const int fd= g_io_channel_unix_get_fd(source);

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        fprintf(stderr, "Hangup: %d\n", condition & G_IO_HUP);
        fprintf(stderr, "IO Err: %d\n", condition & G_IO_ERR);

        fprintf(stderr, "Cannot\n");
        return FALSE;
    }

    fprintf(stderr, "Truing to get output\n");

    const ssize_t bytes_read= read(fd, buff, sizeof(buff) - 1);
    if (bytes_read > 0) {
        buff[bytes_read]= '\0';
        fprintf(stderr,"Got output %s\n", buff);
        output_log(buff);
    } else {
        g_error("Error reading child stdout");
        return FALSE;
    }

    return TRUE;
}

static void run_button_callback(GtkButton* button, gpointer data) {
    show_log("Run process request sent\n");

    queueb_push_blocking(&action_q, create_action(ACTION_CF_CONTINUE, (ACTION_DATA){.NO_DATA= 0}));
}

static void step_out_button_callback(GtkButton* button, gpointer data) {
    queueb_push_blocking(&action_q, create_action(ACTION_CF_STEP_OUT, (ACTION_DATA){.NO_DATA= 0}));
}

static void step_into_button_callback(GtkButton* button, gpointer data) {
    queueb_push_blocking(&action_q, create_action(ACTION_CF_STEP_INTO, (ACTION_DATA){.NO_DATA= 0}));
}

static void step_over_button_callback(GtkButton* button, gpointer data) {
    queueb_push_blocking(&action_q, create_action(ACTION_CF_STEP_OVER, (ACTION_DATA){.NO_DATA= 0}));
}

static void terminal_input_callback(GtkEntry* entry, gpointer data) {
    const char* input= gtk_editable_get_text(GTK_EDITABLE(entry));

    if (input && input[0] != '\0') {
        show_log("> %s\n", input);

        ssize_t res= write(tui_pipe[1], input, strlen(input));
        ssize_t res2= write(tui_pipe[1], "\n", 1);

        show_log("Wrote to file %zu bytes\n", res + res2);
        gtk_editable_set_text(GTK_EDITABLE(entry), "");
    }
}

static GtkWidget* create_terminal_input() {
    GtkWidget* cmd_input= gtk_entry_new();
    gtk_widget_set_hexpand(cmd_input, TRUE);

    gtk_entry_set_placeholder_text(GTK_ENTRY(cmd_input), ">");

    g_signal_connect(cmd_input, "activate", G_CALLBACK(terminal_input_callback), NULL);

    return cmd_input;
}

#define COLOUR_RED 238/255.0f, 59/255.0f, 48/255.0f
#define COLOUR_BLUE 59/255.0f, 48/255.0f,  238/255.0f
#define COLOUR_PURPLE 238/255.0f, 59/255.0f, 200/255.0f
#define COLOUR_GREY 128/255.0f, 128/255.0f, 128/255.0f
static void draw_all_breakpoint_points(GtkDrawingArea* pad, cairo_t* cairo, int width, int height, gpointer data) {
    GtkAdjustment* vadj= GTK_ADJUSTMENT(data);
    double scroll_y= gtk_adjustment_get_value(vadj);
    cairo_translate(cairo, 0, -scroll_y);

    GtkTextView* view= GTK_TEXT_VIEW(disassembly_view);
    GtkTextBuffer* buffer= gtk_text_view_get_buffer(view);

    if (!address_to_mark) return;

    for (int i = 0; i < bp_info.pos; ++i) {
        const BPAddressInfo* info= BPAddressInfo_arr_ptr(&bp_info, i);
        const uintptr_t address= target.target_addr_runtime_to_virtual(info->address);

        LocInfo* loc_info= g_hash_table_lookup(address_to_mark, GINT_TO_POINTER(address));
        if (!loc_info) continue;

        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_mark(buffer, &iter, loc_info->mark);

        if (info->user_bp_count > 0 && info->temp_bp_count > 0) {
            cairo_set_source_rgba(cairo, COLOUR_PURPLE, 1.0);
        } else if (info->user_bp_count > 0) {
            cairo_set_source_rgba(cairo, COLOUR_RED, 1.0);
        } else if (info->temp_bp_count > 0) {
            cairo_set_source_rgba(cairo, COLOUR_BLUE, 1.0);
        } else {
            cairo_set_source_rgba(cairo, COLOUR_GREY, 1.0);
        }

        int line_y, line_height;
        gtk_text_view_get_line_yrange(view, &iter, &line_y, &line_height);
        cairo_arc(cairo, width / 2.0f, line_y + line_height / 2.0f, 6, 0, 2 * G_PI);
        cairo_fill(cairo);
    }
}

static void draw_user_breakpoint_points(GtkDrawingArea* pad, cairo_t* cairo,
    int width, int height, gpointer data) {
    GtkAdjustment* vadj= GTK_ADJUSTMENT(data);
    double scroll_y= gtk_adjustment_get_value(vadj);
    cairo_translate(cairo, 0, -scroll_y);

    GtkTextView* view= GTK_TEXT_VIEW(code_view);
    GtkTextBuffer* buffer= gtk_text_view_get_buffer(view);

    GtkTextIter iter;

    gtk_text_buffer_get_start_iter(buffer, &iter);

    while (!gtk_text_iter_is_end(&iter)) {
        const uint32_t line= gtk_text_iter_get_line(&iter) + 1;

        int line_y, line_height;
        gtk_text_view_get_line_yrange(view, &iter, &line_y, &line_height);

        const LineAddrRes res= target.target_line_to_addr(line);
        if (!res.succ) {
            gtk_text_iter_forward_line(&iter);
            continue;
        }

        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(res.addr);
        const BPAddressInfo* info= BPAddressInfo_arr_search_ie(&bp_info, r_addr);
        const bool has_bp= info && info->user_bp_count > 0;

        if (has_bp) {
            cairo_set_source_rgba(cairo, 238/255.0f, 59/255.0f, 43/255.0f, 1.0);
            cairo_arc(cairo, width / 2.0f, line_y + line_height / 2.0f, 6, 0, 2 * G_PI);
            cairo_fill(cairo);
        }

        gtk_text_iter_forward_line(&iter);
    }
}

static void handle_all_breakpoint_point_click(GtkGestureClick* click, int click_count, double x, double y, gpointer data) {
    GtkAdjustment* vadj= GTK_ADJUSTMENT(data);
    const double scroll_y= gtk_adjustment_get_value(vadj);
    y+= scroll_y;

    GtkTextView* view= GTK_TEXT_VIEW(disassembly_view);
    GtkTextIter iter;
    gtk_text_view_get_line_at_y(view, &iter, y, NULL);
    gtk_text_iter_set_line_offset(&iter, 0);

    GSList* marks= gtk_text_iter_get_marks(&iter);
    if (!marks) {
        show_log("There are no tags at this location\n");
        return;
    }
    const void* address_as_ptr= NULL;
    while (marks) {
        address_as_ptr= g_object_get_data(marks->data, "addr");
        if (address_as_ptr) break;
        marks= marks->next;
    }

    g_slist_free(marks);

    if (!address_as_ptr) {
        show_err("Unable to extract address from marks\n");
        return;
    }
    const uintptr_t address= (uintptr_t)address_as_ptr;

    const uintptr_t r_addr= target.target_addr_virtual_to_runtime(address);
    const BPAddressInfo* info= BPAddressInfo_arr_search_ie(&bp_info, r_addr);
    if (info) {
        show_log("Removing breakpoint\n");
        queueb_push_blocking(&action_q,
            create_action(ACTION_BP_REMOVE, (ACTION_DATA){
                .BP_REMOVE= {
                    .addr= r_addr,
                    .line= -1,
                }
            })
        );
    } else {
        show_log("Adding breakpoint\n");
        queueb_push_blocking(&action_q,
            create_action(ACTION_BP_ADD, (ACTION_DATA){
                .BP_ADD= {
                    .addr= r_addr,
                    .line= -1,
                }
            })
        );
    }
}

static void handle_user_breakpoint_point_click(GtkGestureClick* click, int click_count, double x, double y, gpointer data) {
    show_log("Got click in point\n");
    GtkAdjustment* vadj= GTK_ADJUSTMENT(data);
    double scroll_y= gtk_adjustment_get_value(vadj);
    y+= scroll_y;

    GtkTextView* view= GTK_TEXT_VIEW(code_view);

    GtkTextIter iter;
    gtk_text_view_get_line_at_y(view, &iter, y, NULL);
    const uint32_t line= gtk_text_iter_get_line(&iter) + 1;

    const LineAddrRes res= target.target_line_to_addr(line);
    if (!res.succ) {
        show_err("Unable to find line->address info for line %u\n", line);
        return;
    }

    const uintptr_t r_addr= target.target_addr_virtual_to_runtime(res.addr);
    const BPAddressInfo* info= BPAddressInfo_arr_search_ie(&bp_info, r_addr);
    if (info) {
        show_log("Removing breakpoint\n");
        queueb_push_blocking(&action_q,
            create_action(ACTION_BP_REMOVE, (ACTION_DATA){
                .BP_REMOVE= {
                    .addr= r_addr,
                    .line= line,
                }
            })
        );
    } else {
        show_log("Adding breakpoint\n");
        queueb_push_blocking(&action_q,
            create_action(ACTION_BP_ADD, (ACTION_DATA){
                .BP_ADD= {
                    .addr= r_addr,
                    .line= line,
                }
            })
        );
    }
}

gboolean update_user_breakpoints(gpointer data) {
    gtk_widget_queue_draw(GTK_WIDGET(user_bps));

    return false;
}

gboolean update_breakpoints(gpointer data) {
    gtk_widget_queue_draw(GTK_WIDGET(all_bps));

    return false;
}

gboolean update_breakpoint_displays(gpointer data) {
    gtk_widget_queue_draw(GTK_WIDGET(user_bps));
    gtk_widget_queue_draw(GTK_WIDGET(all_bps));

    return false;
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
    g_signal_connect(run_button, "clicked", G_CALLBACK(run_button_callback), NULL);

    GtkWidget* step_over_button= gtk_button_new_from_icon_name("go-next-symbolic");
    gtk_widget_set_tooltip_text(step_over_button, "Step over");
    g_signal_connect(step_over_button, "clicked", G_CALLBACK(step_over_button_callback), NULL);

    GtkWidget* step_into_button= gtk_button_new_from_icon_name("go-down-symbolic");
    gtk_widget_set_tooltip_text(step_into_button, "Step into");
    g_signal_connect(step_into_button, "clicked", G_CALLBACK(step_into_button_callback), NULL);

    GtkWidget* step_out_button= gtk_button_new_from_icon_name("go-up-symbolic");
    gtk_widget_set_tooltip_text(step_out_button, "Step out");
    g_signal_connect(step_out_button, "clicked", G_CALLBACK(step_out_button_callback), NULL);

    GtkWidget* buttons= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_append(GTK_BOX(buttons), step_over_button);
    gtk_box_append(GTK_BOX(buttons), step_into_button);
    gtk_box_append(GTK_BOX(buttons), step_out_button);
    gtk_box_append(GTK_BOX(buttons), run_button);

    gtk_widget_set_halign(buttons, GTK_ALIGN_END);
    gtk_widget_set_hexpand(run_button, TRUE);

    gtk_box_append(GTK_BOX(top_ui), menu_bar);
    gtk_box_append(GTK_BOX(top_ui), buttons);

    code_view= gtk_text_view_new();
    gtk_widget_set_hexpand(code_view, TRUE);
    gtk_widget_set_vexpand(code_view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(code_view), false);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(code_view), TRUE);

    code_buffer= gtk_text_view_get_buffer(GTK_TEXT_VIEW(code_view));
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


    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(code_view), 20);

    user_bps= gtk_drawing_area_new();
    gtk_widget_set_size_request(user_bps, 20, -1);

    code_scroller= gtk_scrolled_window_new();
    gtk_widget_set_hexpand(code_scroller, TRUE);
    gtk_widget_set_vexpand(code_scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(code_scroller), code_view);

    GtkWidget* code_hbox= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(code_hbox), user_bps);
    gtk_box_append(GTK_BOX(code_hbox), code_scroller);

    GtkAdjustment* v_adj= gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(code_scroller));
    g_signal_connect_swapped(v_adj, "value-changed", G_CALLBACK(gtk_widget_queue_draw), user_bps);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(user_bps), draw_user_breakpoint_points, v_adj, NULL);

    GtkGesture* click= gtk_gesture_click_new();
    gtk_widget_add_controller(user_bps, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(
        click, "pressed",
        G_CALLBACK(handle_user_breakpoint_point_click),
        v_adj
    );

    disassembly_view= gtk_text_view_new();
    gtk_widget_set_hexpand(disassembly_view, TRUE);
    gtk_widget_set_vexpand(disassembly_view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(disassembly_view), false);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(disassembly_view), TRUE);

    disassembly_scroller= gtk_scrolled_window_new();
    // gtk_widget_set_hexpand(disassembly_scroller, TRUE);
    gtk_widget_set_vexpand(disassembly_scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(disassembly_scroller), disassembly_view);

    all_bps= gtk_drawing_area_new();
    gtk_widget_set_size_request(all_bps, 20, -1);

    memory_view= gtk_text_view_new();
    gtk_widget_set_hexpand(memory_view, TRUE);
    gtk_widget_set_vexpand(memory_view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(memory_view), false);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(memory_view), TRUE);

    GtkWidget* memory_scroller= gtk_scrolled_window_new();
    gtk_widget_set_hexpand(memory_scroller, TRUE);
    gtk_widget_set_vexpand(memory_scroller, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(memory_scroller), memory_view);

    GtkAdjustment* vadj= gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(disassembly_scroller));
    gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(memory_scroller), vadj);

    GtkWidget* diss_hbox= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(diss_hbox), all_bps);
    gtk_box_append(GTK_BOX(diss_hbox), disassembly_scroller);
    gtk_box_append(GTK_BOX(diss_hbox), memory_scroller);

    v_adj= gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(disassembly_scroller));
    g_signal_connect_swapped(v_adj, "value-changed", G_CALLBACK(gtk_widget_queue_draw), all_bps);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(all_bps), draw_all_breakpoint_points, v_adj, NULL);

    click= gtk_gesture_click_new();
    gtk_widget_add_controller(all_bps, GTK_EVENT_CONTROLLER(click));
    g_signal_connect(
        click, "pressed",
        G_CALLBACK(handle_all_breakpoint_point_click),
        v_adj
    );

    gtk_box_append(GTK_BOX(code_box), code_hbox);
    gtk_box_append(GTK_BOX(code_box), diss_hbox);

    GtkWidget* terminal_scroller= create_terminal_scroller();
    GtkWidget* terminal_input= create_terminal_input();
    GtkWidget* output_scroller= create_output_scroller();
    GtkWidget* terminal= gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(terminal), terminal_scroller);
    gtk_box_append(GTK_BOX(terminal), terminal_input);

    GtkWidget* terminals= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    gtk_box_append(GTK_BOX(terminals), terminal);
    gtk_box_append(GTK_BOX(terminals), output_scroller);

    gtk_box_append(GTK_BOX(box), code_box);
    gtk_box_append(GTK_BOX(box), terminals);

    GtkEventController* controller= gtk_event_controller_key_new();
    g_signal_connect_object(controller, "key-pressed", G_CALLBACK(event_key_pressed_cb), code_view, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(window, controller);

    GIOChannel* channel = g_io_channel_unix_new(stdio_pipe[0]);
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_buffered(channel, FALSE);
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP, read_target_output_pipe, NULL);

    gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_present(GTK_WINDOW(window));
}

void goto_diss_addr(const uintptr_t addr) {
    GtkTextIter iter;
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(disassembly_view));

    LocInfo* info= g_hash_table_lookup(address_to_mark, GINT_TO_POINTER(addr));
    if (!info) {
        show_err("Unable to find mark associated with address %#lx\n", addr);
        return;
    }

    gtk_text_buffer_get_iter_at_mark(buff, &iter, info->mark);
    gtk_text_iter_forward_to_line_end(&iter);
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(disassembly_view), &iter, 0.0, TRUE, 0.5, 0.5);
}

void goto_line(const int line) {
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_line(code_buffer, &iter, line - 1);
    gtk_text_iter_forward_to_line_end(&iter);

    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(code_view), &iter, 0.0, TRUE, 0.5, 0.5);
}

gboolean goto_line_idle(gpointer data) {
    const uint32_t line= (uint32_t)data;
    goto_line(line);

    return FALSE;
}

gboolean hit_addr(gpointer data) {
    show_log("Hit address\n");

    const uintptr_t addr= GPOINTER_TO_SIZE(data);
    highlight_diss_addr(addr);
    goto_diss_addr(addr);

    const AddrLineRes res= target.target_addr_to_line(addr);
    if (!res.succ) return false;

    highlight_code_line(res.line);
    goto_line(res.line);

    return false;
}

gboolean hit_line(gpointer data) {
    show_log("hit line\n");
    const uint32_t line= GPOINTER_TO_SIZE(data);
    highlight_code_line(line);
    goto_line(line);

    const LineAddrRes res= target.target_line_to_addr(line);
    if (!res.succ) return false;

    highlight_diss_addr(res.addr);
    goto_diss_addr(res.addr);

    return false;
}

ByteStream stream_of_vsection(const VSection* section) {
    ByteStream stream;
    stream.raw_stream= section->data;
    stream.pointer= 0;
    stream.max_pointer= section->size << 3;
    stream.size= section->size << 3;

    return stream;
}

ByteStream stream_of_sub(VSub sub, const VSection* section) {
    ByteStream stream;
    stream.raw_stream= section->data + (sub.vaddr_start - section->vaddr_start);
    stream.pointer= 0;
    stream.max_pointer= (sub.vaddr_end - sub.vaddr_start) << 3;
    stream.size= stream.max_pointer;

    return stream;
}

void fill_disassembly_view() {
    init();
    address_to_mark= g_hash_table_new(g_direct_hash, g_direct_equal);

    const VSection text= target.target_get_text_section();
    top_stream= stream_of_vsection(&text);

    newline();
    SubIter iter= (SubIter) {.idx= 0};
    bool succ;

    GtkTextBuffer* buffer= gtk_text_view_get_buffer(GTK_TEXT_VIEW(disassembly_view));
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    char buff[100];
    VSub sub= target.target_get_next_sub(&iter, &succ);

    while (succ) {
        top_stream= stream_of_sub(sub, &text);

        uintptr_t base= sub.vaddr_start;
        const char* out;

        while (base < sub.vaddr_end && disassemble(&out)) {
            uintptr_t new_base= sub.vaddr_start + (top_stream.pointer >> 3);;

            GtkTextMark* mark= gtk_text_buffer_create_mark(buffer, NULL, &end_iter, TRUE);
            g_object_set_data(G_OBJECT(mark), "addr", GUINT_TO_POINTER(base));
            LocInfo* info= alloc_loc_info(mark, base, new_base - base);
            g_hash_table_insert(address_to_mark, GUINT_TO_POINTER(base), info);

            snprintf(buff, sizeof(buff), "%#lx: %s", base, out);
            gtk_text_buffer_insert(buffer, &end_iter, buff, -1);

            // printf("%#lx: %s\n", base, out);
            base= new_base;

            if (base >= sub.vaddr_end) {
                snprintf(buff, sizeof(buff), " -- sub %s END", sub.subprog_name);
                gtk_text_buffer_insert(buffer, &end_iter, buff, -1);
            }

            gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
        }

        const uintptr_t end= sub.vaddr_end;
        for (int i = base; i < end; ++i) {
            GtkTextMark* mark= gtk_text_buffer_create_mark(buffer, NULL, &end_iter, TRUE);
            LocInfo* info= alloc_loc_info(mark, base, 1);
            g_hash_table_insert(address_to_mark, GUINT_TO_POINTER(base), info);

            snprintf(buff, sizeof(buff), "%#lx: 0x%.2x ???", base, text.data[base - text.vaddr_start]);
            gtk_text_buffer_insert(buffer, &end_iter, buff, -1);
            base++;

            if (i == end - 1) {
                snprintf(buff, sizeof(buff), " -- sub %s END", sub.subprog_name);
                gtk_text_buffer_insert(buffer, &end_iter, buff, -1);
            }

            gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
        }

        sub= target.target_get_next_sub(&iter, &succ);
        if (!succ) break;
        for (uintptr_t i = base; i < sub.vaddr_start; ++i) {
            GtkTextMark* mark= gtk_text_buffer_create_mark(buffer, NULL, &end_iter, TRUE);
            LocInfo* info= alloc_loc_info(mark, base, 1);
            g_hash_table_insert(address_to_mark, GUINT_TO_POINTER(base), info);

            snprintf(buff, sizeof(buff), "%#lx: ???\n", base);
            gtk_text_buffer_insert(buffer, &end_iter, buff, -1);
            base++;
        }
            // printf("%#lx: Disassembly failed\n", base);
    }
}

gboolean update_target_data(gpointer data) {
    fill_disassembly_view();
    fill_memory_view();

    return false;
}

gboolean update_breakpoint_memory(gpointer data) {
    fill_memory_view();

    return false;
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

int create_gui(int pipe[2]) {
    stdio_pipe= pipe;

    app= gtk_application_new ("org.anura.main", G_APPLICATION_FLAGS_NONE);
    create_mappings();

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    const int status= g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return status;
}

// int main(const int argc, char **argv) {
//     if (argc > 1) perror("Command line arguments are currently ignored");
//
//     create_gui();
// }
