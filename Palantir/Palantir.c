#include "Palantir/Palantir.h"

#include <gtk/gtk.h>

#include "break_on_cause.h"
#include "Helper_Math.h"
#include "main.h"
#include "MtDoom.h"
#include "Target.h"
#include "breaksave/break_save.h"
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

GtkWidget* code_box;

GtkWidget* terminal_view;
GtkWidget* output_view;

GListStore* stack_store;

GtkApplication* app;

GtkTextTag* user_bp_hit_highlight= NULL;
size_t user_bp_hit_highlight_line= -1;

GtkTextTag* all_bp_hit_highlight= NULL;
size_t all_bp_hit_highlight_line= -1;

GtkTextTag* line_highlight= NULL;
size_t line_highlight_line= -1;

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

void highlight_code_line_as_info(uint32_t line) {
    if (!line_highlight) line_highlight= gtk_text_buffer_create_tag(
        code_buffer,
        "line-highlight",
        "paragraph-background", "#7bb7ef",
        NULL
    );

    if (line_highlight_line != -1) {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(code_buffer, &start);
        gtk_text_buffer_get_end_iter(code_buffer, &end);
        gtk_text_buffer_remove_tag(code_buffer, line_highlight, &start, &end);
        line_highlight_line= -1;
    }

    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_line(code_buffer, &start, line - 1);
    end= start;
    gtk_text_iter_forward_to_line_end(&end);

    gtk_text_buffer_apply_tag(code_buffer, line_highlight, &start, &end);

    line_highlight_line= line - 1;
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

G_DECLARE_FINAL_TYPE(TracedFrameObject, traced_frame_object, TRACED, FRAME_OBJECT, GObject)
struct _TracedFrameObject {
    GObject parent_instance;
    TracedFrame* frame;
};
G_DEFINE_TYPE(TracedFrameObject, traced_frame_object, G_TYPE_OBJECT)

static void traced_frame_object_class_init(TracedFrameObjectClass* class) {}
static void traced_frame_object_init(TracedFrameObject* self) {}
TracedFrameObject* traced_frame_object_new(TracedFrame* frame) {
    TracedFrameObject* obj= g_object_new(traced_frame_object_get_type(), NULL);
    obj->frame= frame;
    return obj;
}

static GListModel* create_traced_frame_child_model(gpointer item, gpointer user_data) {
    TracedFrameObject* obj= item;
    TracedFrame* frame= obj->frame;

    if (frame->links.pos == 0)
        return NULL;

    GListStore* store= g_list_store_new(traced_frame_object_get_type());

    for (int i = 0; i < frame->links.pos; ++i) {
        TracedFrame* child= TracedFrame_vec_get_unsafe(&frame->links, i);
        g_list_store_append(store, traced_frame_object_new(child));
    }

    return G_LIST_MODEL(store);
}

static void save_tree_setup(
    GtkListItemFactory* factory,
    GtkListItem* list_item,
    gpointer data
) {
    GtkWidget* expander= gtk_tree_expander_new();
    GtkWidget* label= gtk_label_new(NULL);

    gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
    gtk_list_item_set_child(list_item, expander);
}

static void save_tree_bind(
    GtkListItemFactory *factory,
    GtkListItem *list_item,
    gpointer data
) {
    GtkTreeListRow* row= gtk_list_item_get_item(list_item);
    TracedFrameObject* obj= gtk_tree_list_row_get_item(row);

    GtkWidget* expander= gtk_list_item_get_child(list_item);
    GtkWidget* label= gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));

    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander), row);

    char buffer[128];
    snprintf(buffer, sizeof(buffer),
        "CFA: %#lx instance of %s", obj->frame->frame.cfa, obj->frame->frame.sub->subprog_name);

    gtk_label_set_text(GTK_LABEL(label), buffer);
}


static GListModel* create_child_model(gpointer item, gpointer user_data) {
    TreeNodeObject* obj= item;
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

static void setup(
    GtkListItemFactory *factory,
    GtkListItem *list_item,
    gpointer data
) {
    GtkWidget* expander = gtk_tree_expander_new();
    GtkWidget* label = gtk_label_new(NULL);

    gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
    gtk_list_item_set_child(list_item, expander);
}

static void bind(
    GtkListItemFactory *factory,
    GtkListItem *list_item,
    gpointer data
) {
    GtkTreeListRow* row= gtk_list_item_get_item(list_item);
    TreeNodeObject* obj= gtk_tree_list_row_get_item(row);

    GtkWidget* expander= gtk_list_item_get_child(list_item);
    GtkWidget* label= gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));

    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander), row);

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "CFA: %p instance of %s", (void*)obj->node->frame.cfa, obj->node->frame.sub->subprog_name);

    gtk_label_set_text(GTK_LABEL(label), buffer);
}

void var_setup_callback(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
    GtkWidget* label= gtk_label_new(NULL);
    gtk_list_item_set_child(item, label);
}

void var_bind_callback(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer user_data) {
    GtkWidget* label= gtk_list_item_get_child(item);
    const char* text= gtk_string_object_get_string(
        gtk_list_item_get_item(item)
    );
    gtk_label_set_text(GTK_LABEL(label), text);
}

static int highlight_and_goto_line(gpointer data) {
    uint32_t line= GPOINTER_TO_INT(data);

    goto_line(line);
    highlight_code_line_as_info(line);

    return false;
}

GtkWidget* var_box;
GtkWidget* point_info_box;

static int var_search_cmp(const void* a, const void* b) {
    const uint64_t ref= (uint64_t)a;
    const VVarInstance* inst= b;

    if (ref < inst->var->ref) return -1;
    if (ref > inst->var->ref) return 1;
    return 0;
}

static int var_sort_cmp(const void* a, const void* b) {
    const VVarInstance* var_a= a;
    const VVarInstance* var_b= b;

    if (var_a->var->ref < var_b->var->ref) return -1;
    if (var_a->var->ref > var_b->var->ref) return 1;
    return 0;
}

void add_label(GtkWidget* box, char* format, ...) {
    va_list args;
    va_start(args, format);

    char* str= g_strdup_vprintf(format, args);
    GtkWidget* label= gtk_label_new(str);
    free(str);

    gtk_box_append(GTK_BOX(box), label);

    va_end(args);
}

static void break_save_point_selected(GtkListBox* box, GtkListBoxRow* row, gpointer user_data) {
    if (!row) return;

    GtkWidget* child= gtk_list_box_row_get_child(row);

    gpointer point= g_object_get_data(G_OBJECT(child), "point");
    PointInstance* p= point;

    AddrLineRes res= target.target_addr_to_line(p->point->addr);
    if (!res.succ) return;
    g_idle_add(highlight_and_goto_line, GUINT_TO_POINTER(res.line));

    TracedFrame* frame= user_data;
    GtkWidget* existing;
    while (existing= gtk_widget_get_first_child(var_box), existing != NULL) {
        gtk_box_remove(GTK_BOX(var_box), existing);
    }

    while (existing= gtk_widget_get_first_child(point_info_box), existing != NULL) {
        gtk_box_remove(GTK_BOX(point_info_box), existing);
    }

    add_label(point_info_box, "Type: %s", POINT_TYPE_STRS[p->point->type]);
    add_label(point_info_box, "Virtual: %#lx", p->point->addr);
    add_label(point_info_box, "Runtime: %#lx", target.target_addr_virtual_to_runtime(p->point->addr));

    switch (p->type) {
        case POINT_TYPE_DF: {
            DFPointInstance* df= (DFPointInstance*)p;
            DF_POINT* df_point= (DF_POINT*)df->base.point;
            add_label(point_info_box, "Reason: %s", DF_POINT_REASON_STRS[df_point->reason]);
            add_label(point_info_box, "Die offset: %#lx", df_point->die_offset);
            break;
        }
        case POINT_TYPE_CF: {
            CFPointInstance* cf= (CFPointInstance*)p;
            CF_POINT* cf_point= (CF_POINT*)cf->base.point;
            add_label(point_info_box, "Reason: %s", CF_POINT_REASON_STRS[cf_point->reason]);
            break;
        }
        case POINT_TYPE_SENTINEL:
            break;
    }

    const VSub* sub= frame->frame.sub;
    VVarInstanceArray vars= VVarInstance_arr_construct(frame->frame.args.pos + sub->vars.pos);
    for (int i = 0; i < frame->frame.args.pos; ++i) {
        VVarInstance_arr_add(&vars, VVarInstance_arr_get(&frame->frame.args, i));
    }

    for (int i = 0; i < sub->vars.pos; ++i) {
        VVarInstance_arr_add(&vars, (VVarInstance) {
            .value= (Value) {.type= VALUE_NONE},
            .var= VVar_vec_get_unsafe(&sub->vars, i)
        });
    }

    arr_sort((Array*)&vars, var_sort_cmp);

    for (int i = 0; i < frame->points.pos; ++i) {
        PointInstance* pi= PointInstance_vec_get_unsafe(&frame->points, i);
        if (pi->type == POINT_TYPE_DF) {
            DFPointInstance* df= (DFPointInstance*)pi;
            DF_POINT* df_point= (DF_POINT*)df->base.point;

            if (df_point->reason == DF_REASON_VAR_ASSIGN) {
                uint64_t ref= df_point->die_offset;

                VVarInstance* inst= arr_search_e((Array*)&vars, (void*)ref, var_search_cmp);
                if (!inst) continue;

                inst->value= df->value;
            }
        }

        if (pi == p) break;
    }

    GtkStringList* var_model = gtk_string_list_new(NULL);
    for (int i = 0; i < vars.pos; ++i) {
        VVarInstance* inst= VVarInstance_arr_ptr(&vars, i);
        char* stra= g_strdup_printf("Local var %s: ", inst->var->name);
        const char* strb= target.target_create_var_instance_string(inst);
        char* total= g_strconcat(stra, strb, NULL);
        gtk_string_list_append(var_model, total);
        free(stra);
        free(total);
    }


    GtkListItemFactory* var_factory= gtk_signal_list_item_factory_new();
    g_signal_connect(var_factory, "setup", G_CALLBACK(var_setup_callback), NULL);
    g_signal_connect(var_factory, "bind", G_CALLBACK(var_bind_callback), NULL);

    GtkWidget* var_list= gtk_list_view_new(
        GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(var_model))),
        var_factory
    );

    GtkWidget* var_title = gtk_label_new("Variables:");

    gtk_box_append(GTK_BOX(var_box), var_title);
    gtk_box_append(GTK_BOX(var_box), var_list);
}

static void break_save_detail_change(GtkSelectionModel* selection, guint pos, guint n, gpointer data) {
    GtkWidget* cont= GTK_WIDGET(data);

    GtkWidget* existing;
    while (existing= gtk_widget_get_first_child(cont), existing != NULL) {
        gtk_box_remove(GTK_BOX(cont), existing);
    }

    GtkTreeListRow* selected_row= gtk_single_selection_get_selected_item(GTK_SINGLE_SELECTION(selection));
    if (!selected_row) return;
    TracedFrameObject* selected= gtk_tree_list_row_get_item(selected_row);
    if (!selected) return;

    TracedFrame* node= selected->frame;

    GtkWidget* list_box= gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    gtk_box_append(GTK_BOX(cont), list_box);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_SINGLE);

    var_box= gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    point_info_box= gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

    GtkWidget* expander= gtk_expander_new("Point List");

    GtkWidget* marker_list_box= gtk_list_box_new();
    gtk_widget_set_margin_start(marker_list_box, 20);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(marker_list_box), GTK_SELECTION_SINGLE);

    g_signal_connect(marker_list_box, "row-selected", G_CALLBACK(break_save_point_selected), node);

    for (int i = 0; i < node->points.pos; ++i) {
        PointInstance* inst= PointInstance_vec_get_unsafe(&node->points, i);
        if (!inst) continue;

        char* str= g_strdup_printf("[%#lx] Point instance %s ", inst->point->addr, POINT_TYPE_STRS[inst->type]);

        switch (inst->type) {
            case POINT_TYPE_DF: {
                DFPointInstance* df= (DFPointInstance*)inst;
                DF_POINT* point= (DF_POINT*)df->base.point;

                char* str2= g_strconcat(str, DF_POINT_REASON_STRS[point->reason], NULL);
                free(str);
                str= str2;
                break;
            }
            case POINT_TYPE_CF: {
                CFPointInstance* cf= (CFPointInstance*)inst;
                CF_POINT* point= (CF_POINT*)cf->base.point;

                char* str2= g_strconcat(str, CF_POINT_REASON_STRS[point->reason], NULL);
                free(str);
                str= str2;
                break;
            }
            case POINT_TYPE_SENTINEL:
                break;
        }

        GtkWidget* row = gtk_list_box_row_new();

        GtkWidget* label= gtk_label_new(str);
        g_object_set_data(G_OBJECT(label), "point", inst);
        // gtk_label_set_selectable(GTK_LABEL(label), TRUE);

        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(marker_list_box), row);

        g_free(str);
    }

    gtk_expander_set_child(GTK_EXPANDER(expander), marker_list_box);
    gtk_list_box_append(GTK_LIST_BOX(list_box), expander);

    gtk_list_box_append(GTK_LIST_BOX(list_box), point_info_box);
    gtk_list_box_append(GTK_LIST_BOX(list_box), var_box);
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

    GtkWidget* list_box= gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "boxed-list");
    gtk_box_append(GTK_BOX(cont), list_box);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);

    char* cfa_str= g_strdup_printf("CFA: %#lx - %#lx", node->frame.cfa, node->frame.end_stack_pointer);
    gtk_list_box_append(GTK_LIST_BOX(list_box), gtk_label_new(cfa_str));
    g_free(cfa_str);

    GtkWidget* expander= gtk_expander_new("Markers List");
    GtkWidget* marker_list_box= gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(marker_list_box, 20);

    if (node->markers.arr) {
        for (size_t i = 0; i < node->markers.pos; i++) {
            const Marker* marker= Marker_vec_get_unsafe(&node->markers, i);
            if (!marker) continue;
            char* str= g_strdup_printf("Marker @ %#lx prev: %p Target value %s %s: ",
                marker->pc, marker->prev,
                TARGET_VALUE_TYPE_STRS[marker->target_value.type],
                marker->target_value.inverted ? "(INVERTED)" : ""
            );
            switch (marker->target_value.type) {
                case TARGET_VALUE_LOGICAL: {
                    char* str2= g_strconcat(str, "true", NULL);
                    free(str);
                    str= str2;
                    break;
                }
                case TARGET_VALUE_VALUE: {
                    char* str2= g_strdup_printf("%ld", marker->target_value.data.value);
                    char* str3= g_strconcat(str, str2, NULL);
                    free(str);
                    free(str2);
                    str= str3;
                    break;
                }
                case TARGET_VALUE_RULES:
                    for (int j = 0; j < marker->target_value.data.rules.pos; ++j) {
                        TargetRule* rule= TargetRule_arr_ptr(&marker->target_value.data.rules, j);
                        char* str2= g_strdup_printf("%s %s %ld", j != 0 ? " AND " : "", OPERATOR_STRS[rule->op], rule->value);
                        char* str3= g_strconcat(str, str2, NULL);
                        free(str);
                        free(str2);
                        str= str3;
                    }
                    break;
                case TARGET_VALUE_RANGE: {
                    char* str2= g_strdup_printf("%ld-%ld", marker->target_value.data.range.start, marker->target_value.data.range.end);
                    char* str3= g_strconcat(str, str2, NULL);
                    free(str);
                    free(str2);
                    str= str3;
                    break;
                }
                case TARGET_VALUE_ANY: {
                    char* str2= g_strconcat(str, "ANY", NULL);
                    free(str);
                    str= str2;
                    break;
                }
                case TARGET_VALUE_COUNT:
                default:
                    assert(false);
            }

            gtk_box_append(GTK_BOX(marker_list_box), gtk_label_new(str));
            g_free(str);
        }
    }

    gtk_expander_set_child(GTK_EXPANDER(expander), marker_list_box);
    gtk_list_box_append(GTK_LIST_BOX(list_box), expander);

    GtkStringList* var_model = gtk_string_list_new(NULL);
    if (node->frame.end_stack_pointer != 0) {
        for (int i = 0; i < node->frame.vars.pos; ++i) {
            VVarInstance* inst= VVarInstance_arr_ptr(&node->frame.vars, i);
            char* stra= g_strdup_printf("Local var %s: ", inst->var->name);
            const char* strb= target.target_create_var_instance_string(inst);
            char* total= g_strconcat(stra, strb, NULL);
            gtk_string_list_append(var_model, total);
            free(stra);
            free(total);
        }
    }

    GtkListItemFactory* var_factory= gtk_signal_list_item_factory_new();
    g_signal_connect(var_factory, "setup", G_CALLBACK(var_setup_callback), NULL);
    g_signal_connect(var_factory, "bind", G_CALLBACK(var_bind_callback), NULL);

    GtkWidget* var_list= gtk_list_view_new(
        GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(var_model))),
        var_factory
    );

    GtkWidget* var_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget* var_title = gtk_label_new("Local variables:");

    gtk_box_append(GTK_BOX(var_box), var_title);
    gtk_box_append(GTK_BOX(var_box), var_list);
    gtk_list_box_append(GTK_LIST_BOX(list_box), var_box);

    GtkStringList* arg_model = gtk_string_list_new(NULL);
    for (int i = 0; i < node->frame.args.pos; ++i) {
        VVarInstance* inst= VVarInstance_arr_ptr(&node->frame.args, i);
        char* stra= g_strdup_printf("Local var %s: ", inst->var->name);
        const char* strb= target.target_create_var_instance_string(inst);
        char* total= g_strconcat(stra, strb, NULL);
        gtk_string_list_append(var_model, total);
        free(stra);
        free(total);
    }

    GtkListItemFactory* arg_factory= gtk_signal_list_item_factory_new();
    g_signal_connect(arg_factory, "setup", G_CALLBACK(var_setup_callback), NULL);
    g_signal_connect(arg_factory, "bind", G_CALLBACK(var_bind_callback), NULL);

    GtkWidget* arg_list= gtk_list_view_new(
        GTK_SELECTION_MODEL(gtk_single_selection_new(G_LIST_MODEL(arg_model))),
        arg_factory
    );

    GtkWidget* arg_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget* arg_title = gtk_label_new("Function arguments:");

    gtk_box_append(GTK_BOX(arg_box), arg_title);
    gtk_box_append(GTK_BOX(arg_box), arg_list);
    gtk_list_box_append(GTK_LIST_BOX(list_box), arg_box);
}

G_DECLARE_FINAL_TYPE(RegInstance, reg_instance, REG, INSTANCE, GObject)
struct _RegInstance {
    GObject parent_instance;
    const char* name;
    const char* value;
};
G_DEFINE_TYPE(RegInstance, reg_instance, G_TYPE_OBJECT)
static void reg_instance_init(RegInstance* item) {}
static void reg_instance_class_init(RegInstanceClass* class) {}

static RegInstance* reg_instance_new(VRegInstance* vreg) {
    RegInstance* reg= g_object_new(reg_instance_get_type(), NULL);
    reg->name= g_strdup(vreg->name);
    reg->value= g_strdup(vreg->value);
    return reg;
}

GListModel* create_reg_instance_model(VRegInstanceArray* instances) {
    GListStore* store= g_list_store_new(reg_instance_get_type());

    for (int i = 0; i < instances->pos; ++i) {
        g_list_store_append(store, reg_instance_new(VRegInstance_arr_ptr(instances, i)));
    }

    return G_LIST_MODEL(store);
}

static void reg_inst_setup_callback(GtkSignalListItemFactory* factory, GObject* item, gpointer data) {
    GtkWidget* label= gtk_label_new(NULL);
    gtk_list_item_set_child(GTK_LIST_ITEM(item), label);
}

static void reg_inst_bind_name_callback(GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data) {
    GtkWidget* label= gtk_list_item_get_child(item);
    GObject* generic= gtk_list_item_get_item(GTK_LIST_ITEM(item));
    RegInstance* reg= REG_INSTANCE(generic);
    gtk_label_set_text(GTK_LABEL(label), reg->name);
}

static void reg_inst_bind_value_callback(GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data) {
    GtkWidget* label= gtk_list_item_get_child(item);
    GObject* generic= gtk_list_item_get_item(GTK_LIST_ITEM(item));
    RegInstance* reg= REG_INSTANCE(generic);
    gtk_label_set_text(GTK_LABEL(label), reg->value);
}

gboolean display_all_registers(gpointer data) {
    AllRegs* regs= data;

    VRegInstanceArray instances= target.target_get_all_regs_instance(regs);

    GtkWidget* window= gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Registers");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920/2, 1080/2);

    GListModel* root_model= create_reg_instance_model(&instances);

    GtkSelectionModel* selection= GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(root_model))
    );

    GtkWidget* view= gtk_column_view_new(selection);

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

    GdkDisplay* display= gtk_widget_get_display(view);
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    g_object_unref(provider);

    GtkListItemFactory* factory= gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(reg_inst_setup_callback), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(reg_inst_bind_name_callback), NULL);

    GtkColumnViewColumn* name_col= gtk_column_view_column_new("Name", GTK_LIST_ITEM_FACTORY(factory));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(view), name_col);

    GtkListItemFactory* val_factory= gtk_signal_list_item_factory_new();
    g_signal_connect(val_factory, "setup", G_CALLBACK(reg_inst_setup_callback), NULL);
    g_signal_connect(val_factory, "bind", G_CALLBACK(reg_inst_bind_value_callback), NULL);

    GtkColumnViewColumn* value_col= gtk_column_view_column_new("Value", GTK_LIST_ITEM_FACTORY(val_factory));
    gtk_column_view_append_column(GTK_COLUMN_VIEW(view), value_col);
    gtk_column_view_column_set_expand(value_col, TRUE);

    GtkWidget* scroller= gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), view);

    gtk_window_set_child(GTK_WINDOW(window), scroller);
    gtk_widget_show(window);

    return false;
}

gboolean display_break_save_tree(gpointer root) {
    GtkWidget* window= gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Break on cause result");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920/2, 1080/2);

    GListStore* root_model= g_list_store_new(traced_frame_object_get_type());
    g_list_store_append(root_model, traced_frame_object_new(root));

    GtkTreeListModel* tree_list= gtk_tree_list_model_new(
        G_LIST_MODEL(root_model),
        FALSE,
        FALSE,
        create_traced_frame_child_model,
        NULL,
        NULL
    );

    GtkSelectionModel* selection= GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(tree_list))
    );

    GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(save_tree_setup), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(save_tree_bind), NULL);

    GtkWidget* view = gtk_list_view_new(selection, factory);

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

    g_signal_connect(selection, "selection-changed", G_CALLBACK(break_save_detail_change), details_view);

    gtk_window_present(GTK_WINDOW(window));

    return false;
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

    GtkSelectionModel* selection= GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(tree_list))
    );

    GtkListItemFactory* factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind), NULL);

    GtkWidget* view = gtk_list_view_new(selection, factory);

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

G_DECLARE_FINAL_TYPE(Frame, frame, FRAME, frame, GObject)
struct _Frame{
    GObject parent_instance;

    char* text;
    uintptr_t v_addr;
};
G_DEFINE_TYPE(Frame, frame, G_TYPE_OBJECT)
static void frame_init(Frame* self) {}
static void frame_class_init(FrameClass* class) {}

static Frame* frame_new(StackFrame* stack_frame) {
    Frame* item= g_object_new(frame_get_type(), NULL);

    item->text= g_strdup_printf(
        "Frame @ %#lx (v: %#lx)\n"
        "\tSubroutine: %s:%u",
        stack_frame->pc, stack_frame->v_pc,
        stack_frame->sub->subprog_name, stack_frame->line
    );

    item->v_addr= stack_frame->v_pc;

    return item;
}


gboolean display_stack_trace(gpointer data) {
    Stack* stack= data;

    g_list_store_remove_all(stack_store);

    for (int i = 0; i < stack->pos; ++i) {
        StackFrame* frame= StackFrame_arr_ptr(stack, i);
        Frame* f= frame_new(frame);

        g_list_store_append(stack_store, f);
    }

    return false;
}

typedef struct {
    GtkTextBuffer* mem_buff;
    GtkTextBuffer* diss_buff;
} MemoryViewBuffers;
MemoryViewBuffers mem_buffers;

typedef struct MemoryData {
    Data data;
    LocInfo* info;
} MemoryData;

static void memory_view_add(gpointer key, gpointer value, gpointer data) {
    const LocInfo* info= value;
    const uintptr_t addr= GPOINTER_TO_SIZE(key);

    const Data* text_data= data;
    const VSection text_section= target.target_get_text_section();

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(mem_buffers.diss_buff, &iter, info->mark);
    const int line= gtk_text_iter_get_line(&iter);

    const int buff_size= 3 * info->bytes + 1;
    const uintptr_t start_addr_offset= addr - text_section.vaddr_start;
    char* buff= malloc(buff_size);
    for (int i = 0; i < info->bytes; ++i) {
        snprintf(&buff[i * 3], 4, "%.2x ", text_data->raw_data[start_addr_offset + i]);
    }

    gtk_text_buffer_get_iter_at_line(mem_buffers.mem_buff, &iter, line);
    GtkTextIter end= iter;
    gtk_text_buffer_get_end_iter(mem_buffers.mem_buff ,&end);

    gtk_text_buffer_insert(mem_buffers.mem_buff, &iter, buff, -1);
}

static void fill_memory_view(Data* text_data) {
    GtkTextBuffer* buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(memory_view));
    gtk_text_buffer_set_text(buff, "", -1);

    GtkTextBuffer* diss_buff= gtk_text_view_get_buffer(GTK_TEXT_VIEW(disassembly_view));
    const int lines= gtk_text_buffer_get_line_count(diss_buff);

    mem_buffers= (MemoryViewBuffers){
        .diss_buff= diss_buff,
        .mem_buff= buff
    };

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buff, &end);
    for (int i = 0; i < lines; ++i) {
        gtk_text_buffer_insert(buff, &end, "\n", -1);
    }

    g_hash_table_foreach(address_to_mark, memory_view_add, text_data);
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
        queueb_push_blocking(&action_q, create_action(ACTION_AT_OPEN, (ACTION_DATA) {
            .AT_OPEN= {
                .path= path
            }
        }));
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

static void register_button_callback(GtkButton* button, gpointer data) {
    queueb_push_blocking(&action_q, create_action(ACTION_DS_REGS, (ACTION_DATA){.NO_DATA= 0}));
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

static void stack_setup_callback(GtkListItemFactory* factory, GtkListItem* item, gpointer user_data) {
    GtkWidget* label= gtk_label_new(NULL);
    gtk_list_item_set_child(item, label);
}

static void stack_bind_callback(GtkListItemFactory* factory, GtkListItem* item, gpointer user_data) {
    GtkWidget* label= gtk_list_item_get_child(item);
    Frame* frame= gtk_list_item_get_item(item);
    const char* text= frame->text;

    gtk_label_set_text(GTK_LABEL(label), text);
}

static void stack_frame_selected(GtkSelectionModel* model, guint pos, guint item_c, gpointer data) {
    guint selected= gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(model));
    if (selected == GTK_INVALID_LIST_POSITION)
        return;

    GListModel* list= gtk_single_selection_get_model(GTK_SINGLE_SELECTION(model));
    Frame* frame= g_list_model_get_item(list, selected);

    if (!frame) return;

    uintptr_t v_addr= frame->v_addr;
    AddrLineRes res= target.target_addr_to_line(v_addr);
    if (!res.succ) return;

    highlight_code_line_as_info(res.line);
    goto_line(res.line);
}

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window= gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Anura: GUI Debugger");
    gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);

    GtkWidget* box= gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget* top_ui= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(box), top_ui);

    code_box= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

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

    GtkWidget* register_button= gtk_button_new_with_label("regs");
    g_signal_connect(register_button, "clicked", G_CALLBACK(register_button_callback), NULL);

    GtkWidget* buttons= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_append(GTK_BOX(buttons), register_button);
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
    // set_buffer_as_file("/home/jamestbest/test.s", code_buffer);

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

    GtkWidget* stack_trace_scroller= gtk_scrolled_window_new();
    stack_store= g_list_store_new(frame_get_type());
    GtkListItemFactory* stack_factory= gtk_signal_list_item_factory_new();
    g_signal_connect(stack_factory, "setup", G_CALLBACK(stack_setup_callback), NULL);
    g_signal_connect(stack_factory, "bind", G_CALLBACK(stack_bind_callback), NULL);

    GtkSingleSelection* selection= gtk_single_selection_new(G_LIST_MODEL(stack_store));
    g_signal_connect(selection, "selection-changed", G_CALLBACK(stack_frame_selected), NULL);

    GtkWidget* stack_list= gtk_list_view_new(GTK_SELECTION_MODEL(selection), stack_factory);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stack_trace_scroller), stack_list);
    gtk_widget_set_size_request(stack_trace_scroller, 250, -1);

    GtkWidget* terminals= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    gtk_box_append(GTK_BOX(terminals), terminal);
    gtk_box_append(GTK_BOX(terminals), output_scroller);
    gtk_box_append(GTK_BOX(terminals), stack_trace_scroller);

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

ByteStream stream_of_sub(VSub* sub, const VSection* section) {
    ByteStream stream;
    stream.raw_stream= section->data + (sub->vaddr_start - section->vaddr_start);
    stream.pointer= 0;
    stream.max_pointer= (sub->vaddr_end - sub->vaddr_start) << 3;
    stream.size= stream.max_pointer;

    return stream;
}

typedef struct {
    bool success;
    uintptr_t pointer;
} DissRes;

DissRes disassemble_from_to(VSub* sub, uintptr_t base, uintptr_t end, const char* sub_name, GtkTextBuffer* buffer, GtkTextIter* end_iter) {
    char scratch_buff[100];
    const char* out;

    while (base < end && disassemble(&out, base)) {
        const uintptr_t new_base= sub->vaddr_start + (top_stream.pointer >> 3);

        GtkTextMark* mark= gtk_text_buffer_create_mark(buffer, NULL, end_iter, TRUE);
        g_object_set_data(G_OBJECT(mark), "addr", GUINT_TO_POINTER(base));
        LocInfo* info= alloc_loc_info(mark, base, new_base - base);
        g_hash_table_insert(address_to_mark, GUINT_TO_POINTER(base), info);

        snprintf(scratch_buff, sizeof(scratch_buff), "%#lx: %s", base, out);
        gtk_text_buffer_insert(buffer, end_iter, scratch_buff, -1);

        // printf("%#lx: %s\n", base, out);
        base= new_base;

        if (base >= end) {
            snprintf(scratch_buff, sizeof(scratch_buff), " -- sub %s END", sub_name);
            gtk_text_buffer_insert(buffer, end_iter, scratch_buff, -1);
        }

        gtk_text_buffer_insert(buffer, end_iter, "\n", -1);
    }

    return (DissRes) {
        .success= base >= end,
        .pointer= base
    };
}

uintptr_t fill_with_unknown(
    VSection section,
    uintptr_t base,
    uintptr_t end,
    GtkTextBuffer* buffer,
    GtkTextIter* end_iter,
    const char* sub_name,
    ByteStream* stream
) {
    char scratch_buffer[100];
    uintptr_t start= base;
    for (uintptr_t i = base; i < end; ++i) {
        GtkTextMark* mark= gtk_text_buffer_create_mark(buffer, NULL, end_iter, TRUE);
        LocInfo* info= alloc_loc_info(mark, base, 1);
        g_hash_table_insert(address_to_mark, GUINT_TO_POINTER(base), info);

        snprintf(scratch_buffer, sizeof(scratch_buffer), "%#lx: 0x%.2x ???", base, section.data[base - section.vaddr_start]);
        gtk_text_buffer_insert(buffer, end_iter, scratch_buffer, -1);
        base++;

        if (i == end - 1 && sub_name) {
            snprintf(scratch_buffer, sizeof(scratch_buffer), " -- sub %s END", sub_name);
            gtk_text_buffer_insert(buffer, end_iter, scratch_buffer, -1);
        }

        gtk_text_buffer_insert(buffer, end_iter, "\n", -1);
    }

    stream->pointer += (end - start) << 3;
    return base;
}
//
// void fill_cu_disassembly_view(CU* cu) {
//
// }

VECTOR_PROTO(struct FileNode, FileNode)
typedef struct FileNode {
    const char* name;
    gboolean is_folder;
    FileNodeVector children;
} FileNode;
VECTOR_ADD(FileNode, FileNode)

G_DECLARE_FINAL_TYPE(FileObject, file_object, FILE, FILE_OBJECT, GObject)
struct _FileObject {
    GObject parent_instance;
    FileNode* file;
};
G_DEFINE_TYPE(FileObject, file_object, G_TYPE_OBJECT)

static void file_object_class_init(FileObjectClass* class) {}
static void file_object_init(FileObject* self) {}
FileObject* file_object_new(FileNode* file) {
    FileObject* obj= g_object_new(file_object_get_type(), NULL);
    obj->file= file;
    return obj;
}

FileNode* alloc_file_node(const char* name, bool is_folder) {
    FileNode* file= malloc(sizeof(FileNode));
    *file= (FileNode) {
        .children= FileNode_vec_construct(5),
        .is_folder= is_folder,
        .name= name
    };
    return file;
}

static GListModel* create_file_child_model(gpointer item, gpointer user_data) {
    FileObject* obj= item;
    FileNode* file= obj->file;

    if (file->children.pos == 0) return NULL;

    GListStore* store= g_list_store_new(file_object_get_type());

    for (int i = 0; i < file->children.pos; ++i) {
        FileNode* child= FileNode_vec_get_unsafe(&file->children, i);
        FileObject* child_obj= file_object_new(child);
        g_list_store_append(store, child_obj);
    }

    return G_LIST_MODEL(store);
}

FileNode* find_or_add_child(FileNode* existing_root, char* path_part, bool is_file) {
    for (int i = 0; i < existing_root->children.pos; ++i) {
        FileNode* item= FileNode_vec_get_unsafe(&existing_root->children, i);

        if (strcmp(item->name, path_part) == 0) {
            return item;
        }
    }

    FileNode* new= alloc_file_node(path_part, is_file);
    FileNode_vec_add(&existing_root->children, new);

    return new;
}

FileNode* build_file_tree(Vector filepaths) {
    FileNode* root= alloc_file_node("/", true);

    for (int i = 0; i < filepaths.pos; ++i) {
        const char* filepath= vector_get_unsafe(&filepaths, i);
        char** splits= g_strsplit(filepath, "/", -1);

        FileNode* current= root;
        size_t idx= 0;
        while (splits[idx] != NULL) {
            char* part= splits[idx];

            if (part[0] == '\0') {
                idx++;
                continue;
            };

            char* file_part= strdup(part);
            gboolean is_last= splits[idx + 1] == NULL;
            FileNode* file= find_or_add_child(current, file_part, is_last);

            current= file;
            idx++;
        }

        g_strfreev(splits);
    }

    return root;
}

void setup_file_view_callback(GtkListItemFactory* factory, GtkListItem* list_item, gpointer data) {
    GtkWidget* expander= gtk_tree_expander_new();
    GtkWidget* label= gtk_label_new(NULL);

    gtk_tree_expander_set_child(GTK_TREE_EXPANDER(expander), label);
    gtk_list_item_set_child(list_item, expander);
}

void bind_file_view_callback(GtkListItemFactory* factory, GtkListItem* list_item, gpointer data) {
    GtkTreeListRow* row= gtk_list_item_get_item(list_item);
    FileObject* file= gtk_tree_list_row_get_item(row);

    GtkWidget* expander= gtk_list_item_get_child(list_item);
    GtkWidget* label= gtk_tree_expander_get_child(GTK_TREE_EXPANDER(expander));

    gtk_tree_expander_set_list_row(GTK_TREE_EXPANDER(expander), row);

    gtk_label_set_text(GTK_LABEL(label), file->file->name);
}

void fill_file_view() {
    Vector filepaths= target.target_get_all_cu_filenames();
    FileNode* tree= build_file_tree(filepaths);

    GListStore* root_model= g_list_store_new(file_object_get_type());
    g_list_store_append(root_model, file_object_new(tree));

    GtkTreeListModel* file_list= gtk_tree_list_model_new(
        G_LIST_MODEL(root_model),
        FALSE,
        FALSE,
        create_file_child_model,
        NULL,
        NULL
    );

    GtkSelectionModel* selection= GTK_SELECTION_MODEL(
        gtk_single_selection_new(G_LIST_MODEL(file_list))
    );

    GtkListItemFactory* factory= gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_file_view_callback), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_file_view_callback), NULL);

    GtkWidget* view= gtk_list_view_new(selection, factory);

    gtk_box_prepend(GTK_BOX(code_box), view);
    gtk_widget_show(view);
}

void fill_disassembly_view() {
    init();
    address_to_mark= g_hash_table_new(g_direct_hash, g_direct_equal);

    const VSection text= target.target_get_text_section();
    top_stream= stream_of_vsection(&text);

    newline();
    SubIter iter= target.target_get_selected_cu_sub_iter();
    bool succ;

    GtkTextBuffer* buffer= gtk_text_view_get_buffer(GTK_TEXT_VIEW(disassembly_view));
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    VSub* sub= target.target_get_next_sub(&iter);

    while (sub) {
        top_stream= stream_of_sub(sub, &text);

        uintptr_t base= sub->vaddr_start;

        const AddrLineRes max_line_res= target.target_addr_to_line(sub->vaddr_end);
        uint32_t max_line= -1;
        if (max_line_res.succ)
            max_line= max_line_res.line;

        while (true) {
            const DissRes res= disassemble_from_to(sub, base, sub->vaddr_end, sub->subprog_name, buffer, &end_iter);
            base= res.pointer;
            if (res.success) break;

            const AddrLineRes line_res= target.target_addr_to_line(base);
            if (!line_res.succ) {
                break;
            }

            uint32_t line= line_res.line;
            while (true) {
                line++;
                if (line > max_line) break;

                const LineAddrRes addr_res= target.target_line_to_addr(line);
                if (!addr_res.succ) continue;

                const uintptr_t new_base= addr_res.addr;
                base= fill_with_unknown(text, base, new_base, buffer, &end_iter, NULL, &top_stream);
                break;
            }
        }

        base= fill_with_unknown(text, base, sub->vaddr_end, buffer, &end_iter, sub->subprog_name, &top_stream);

        sub= target.target_get_next_sub(&iter);
        if (sub == NULL) break;

        fill_with_unknown(text, base, sub->vaddr_start, buffer, &end_iter, NULL, &top_stream);
    }
}

gboolean update_target_data(gpointer data) {
    target.target_set_selected_cu(target.target_get_main_cu());

    fill_disassembly_view();
    fill_memory_view(data);
    fill_file_view();

    return false;
}

gboolean update_breakpoint_memory(gpointer data) {
    Data* text_data= data;
    fill_memory_view(text_data);

    return false;
}

static void quit_action(GSimpleAction *action, GVariant *parameter, gpointer app) {
    g_application_quit(G_APPLICATION(app));
}

static void create_quit_mapping() {
    GSimpleAction* quit= g_simple_action_new("quit", NULL);
    g_signal_connect(quit, "activate", G_CALLBACK(quit_action), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit));
}

static void create_open_mapping() {
    GSimpleAction* open_action= g_simple_action_new("open", NULL);
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
