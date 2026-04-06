//
// Created by jamestbest on 3/10/26.
//

#include "break_on_cause.h"
#include "break_on_cause_internal.h"

#include <stdint.h>

#include "Vector.h"
#include "Array.h"
#include "IsildursBane.h"
#include "Target.h"
#include "DWARF/Balin.h"
#include "DWARF/Expr_eval.h"
#include "Palantir/Palantir.h"

static void marker_hit_downstream_callback(void* data);
static void marker_hit_upstream_callback(void* data);
static int64_t get_location_data(Location loc);
void capture_all_args(VSub* sub, VVarInstanceArray* arr, uintptr_t cfa);

const char* DEPTYPE_STRS[DEPTYPE_COUNT]= {
    [DEPTYPE_LINEAR]= "DEPTYPE_LINEAR",
    [DEPTYPE_SPLIT]= "DEPTYPE_SPLIT",
    [DEPTYPE_COND]= "DEPTYPE_COND",
    [DEPTYPE_SENTINEL]= "DEPTYPE_SENTINEL",
    [DEPTYPE_HEADER]= "DEPTYPE_HEADER",
    [DEPTYPE_EXPR]= "DEPTYPE_EXPR",
    [DEPTYPE_COLLECT]= "DEPTYPE_COLLECT"
};

const char* OPERATOR_STRS[OP_COUNT]= {
    [OP_EQ]= "==",
    [OP_NE]= "!=",
    [OP_GT]= ">"
};

const char* TARGET_VALUE_TYPE_STRS[TARGET_VALUE_COUNT]= {
    [TARGET_VALUE_ANY]= "ANY VALUE",
    [TARGET_VALUE_RULES]= "RULES",
    [TARGET_VALUE_LOGICAL]= "LOGICAL",
    [TARGET_VALUE_VALUE]= "CONST VALUE",
    [TARGET_VALUE_RANGE]= "RANGE"
};

const TargetValue TARGET_VALUE_BASE= (TargetValue) {
    .type= TARGET_VALUE_ANY
};



DEP_LINEAR dep_t3= MAKE_CONST(0, t3_output, 0x1131)
DEP_HEADER header_t3= MAKE_HEADER(1, 0x1129, dep_t3, NOT_DEAD, NO_PASS);

DEP_LINEAR dep_t2= MAKE_CONST(2, t2_output, 0x1140)
DEP_HEADER header_t2= MAKE_HEADER(3, 0x1138, dep_t2, NOT_DEAD, NO_PASS);

DEP_LINEAR dep_t2_call_in_t1= MAKE_SINGLE(
    4,
    0x1154, 0x5,
    RAX_CODE,
    NOT_DEAD, NO_PASS,
    header_t2, "t2()"
);

DEP_LINEAR dep_t1_return_fail= MAKE_CONST(5, FAIL, 0x1162);
DEP_COND dep_t1_cond_on_t2= MAKE_COND(
    6,
    NOT_DEAD,
    IS_LEFT,
    dep_t2_call_in_t1,
    dep_t1_return_fail
);

DEP_LINEAR dep_t3_call_in_t1= MAKE_SINGLE(
    7,
    0x1169, 0x5,
    RAX_CODE,
    NOT_DEAD,
    NO_PASS,
    header_t3, "t3()"
);
DEP_LINEAR dep_t1_return_success= MAKE_CONST(8, SUCCESS, 0x1177);
DEP_COND dep_t1_cond_on_t3= MAKE_COND(
    9,
    NOT_DEAD,
    IS_LEFT,
    dep_t3_call_in_t1,
    dep_t1_return_success
);

DEP_LINEAR dep_t1_linear_on_fail= MAKE_CONST(10, FAIL, 0x1179);

DEP_SPLIT dep_t1= MAKE_SPLIT(11, NOT_DEAD,
    (DEP_BASE*)&dep_t1_cond_on_t2, \
    (DEP_BASE*)&dep_t1_cond_on_t3, \
    (DEP_BASE*)&dep_t1_linear_on_fail \
);
DEP_HEADER header_t1= MAKE_HEADER(12, 0x1147, dep_t1, NOT_DEAD, NO_PASS);

DEP_LINEAR dep_on_t1= MAKE_SINGLE(13, 0x1198, 0x5, LOC_RAX, NOT_DEAD, NO_PASS, header_t1, "t1()");
DEP_LINEAR const_1= MAKE_CONST(14, FAIL, 0x119d);
DEP_EXPR t1_fail_expr= MAKE_EXPR(15, NOT_DEAD, NO_PASS,
    OP_EQ, dep_on_t1, const_1,
    TYPE_LEFT,
    0x1198, 0x8,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR res_set_as_fail= MAKE_DATA(16, LOC_REG_OFF(RBP_CODE, -0x4, 4), 0x11a9);
DEP_COND main_res_34= MAKE_COND(17, NOT_DEAD, IS_LEFT, t1_fail_expr, res_set_as_fail);


SUB_INFO(main, 0x1180, 0x11ac)
SUB_INFO(t1, 0x1147, 0x117e)
SUB_INFO(t2, 0x1138, 0x1145)
SUB_INFO(t3, 0x1129, 0x1136)

const SubInfo* simple_sub_infos[]= {
    &main_info,
    &t1_info,
    &t2_info,
    &t3_info,
    NULL
};



const SubInfo** sub_infos;

int collect_info_cmp(DEP_COLLECT* a, DEP_COLLECT* b) {
    return a - b;
}


VECTOR_ADD(Marker, Marker)
VECTOR_ADD(struct TreeNode, Node)
ARRAY_ADD_CMP(CollectInfo, CollectInfo, collect_info_cmp, collect)
ARRAY_ADD(TargetRule, TargetRule)

MarkerVector markers;
TreeNode* root;

TreeNode* alloc_node(uint64_t cfa, DEP_BASE* base) {
    TreeNode* node= malloc(sizeof(TreeNode));
    const uintptr_t pc= target.target_get_pc();
    const uintptr_t v_pc= target.target_addr_runtime_to_virtual(pc);
    VSub* sub= target.target_get_vsub_at(v_pc);

    *node= (TreeNode) {
        .cfa= cfa,
        .links= Node_vec_create(),
        .base= base,
        .frame= (StackFrame){
            .cfa= cfa,
            .pc= pc,
            .v_pc= v_pc,
            .sub= sub,
            .args= VVarInstance_arr_create(),
            .vars= VVarInstance_arr_create(),
            .end_stack_pointer= 0,
        },
        .markers= Marker_vec_create(),
        .collections= CollectInfo_arr_construct(0)
    };

    capture_all_args(node->frame.sub, &node->frame.args, node->frame.cfa);

    return node;
}

#define TARGET_LOGICAL(val) (TargetValue) {.type= TARGET_VALUE_LOGICAL, .inverted= !val}
#define TARGET_CONST(val) (TargetValue) {.type= TARGET_VALUE_VALUE, .data.value= val}

uintptr_t marker_cmp(uintptr_t a, uintptr_t b) {
    return a - b;
}

Marker* create_marker(Marker* prev, bool is_dead, DEP_BASE* pos, TargetValue target_value) {
    Marker* marker= malloc(sizeof(Marker));
    *marker= (Marker) {
        .prev= prev,
        .pos= pos,
        .dead= is_dead,
        .target_value= target_value
    };

    return marker;
}

bool is_hittable_type(DEPTYPE type) {
    return type == DEPTYPE_LINEAR || type == DEPTYPE_EXPR;
}

const SubInfo* find_sub_info(uintptr_t addr) {
    const SubInfo* info= sub_infos[0];

    size_t i= 0;
    while (info) {
        if (info->func_start == addr) return info;
        info= sub_infos[++i];
    }

    return NULL;
}

void set_instance_data_from_loc(VLocation loc, VVarInstance* inst, uint8_t size, uintptr_t cfa) {
    const bool gen= size <= 8;
    if (gen) {
        inst->value.type= VALUE_GENERAL_VALUE;
    } else {
        inst->value.type= VALUE_DATA;
    }

    switch (loc.type) {
        case VLOCATION_REGISTER: {
            inst->value.data.general= target.target_get_reg(loc.data.register_id).value.general;
            break;
        }
        case VLOCATION_REG_OFF: {
            const int64_t reg_value= target.target_get_reg(loc.data.reg_off.register_id).value.general;
            const int64_t pos= reg_value + loc.data.reg_off.offset;
            if (!gen) {
                inst->value.data.data= target.target_get_data_runtime(pos, size);
            } else {
                inst->value.data.general= target.target_get_general_data_runtime(pos, size);
            }
            break;
        }
        case VLOCATION_CONST: {
            inst->value.data.general= loc.data.constant;
            break;
        }
        case VLOCATION_ADDR: {
            const int64_t pos= loc.data.vaddr;
            if (!gen) {
                inst->value.data.data= target.target_get_data_runtime(pos, size);
            } else {
                inst->value.data.general= target.target_get_general_data_runtime(pos, size);
            }
            break;
        }
        case VLOCATION_NO_ACCESS: {
            inst->value.type= VALUE_NONE;
            break;
        }
        case VLOCATION_EXPR: {
            const VLocation new_loc= eval_dw_expr_to_location(loc.data.expr, cfa);
            set_instance_data_from_loc(new_loc, inst, size, cfa);
            break;
        }
    }
}

VVarInstance instance_var(VVar* var, uintptr_t cfa) {
    const uint8_t size= get_type_size(var->type);

    VVarInstance inst;
    inst.var= var;

    set_instance_data_from_loc(var->loc, &inst, size, cfa);

    return inst;
}

void capture_all_vars(VSub* sub, VVarInstanceArray* arr, uintptr_t cfa) {
    for (int i = 0; i < sub->vars.pos; ++i) {
        VVar* var= VVar_arr_ptr(&sub->vars, i);
        VVarInstance_arr_add(arr, instance_var(var, cfa));
    }
}

void capture_all_args(VSub* sub, VVarInstanceArray* arr, uintptr_t cfa) {
    for (int i = 0; i < sub->params.pos; ++i) {
        VVar* var= VVar_arr_ptr(&sub->params, i);
        VVarInstance_arr_add(arr, instance_var(var, cfa));
    }
}

void func_end_callback(void* data) {
    Marker* marker= data;
    const DEP_HEADER* header= (DEP_HEADER*)marker->pos;

    TreeNode* node= marker->node;
    node->frame.end_stack_pointer= target.target_get_reg(RSP_CODE).value.general;

    const size_t size= node->frame.cfa - node->frame.end_stack_pointer + 1;
    node->frame.data= target.target_get_data_runtime(node->frame.cfa, size).raw_data;

    capture_all_vars(node->frame.sub, &node->frame.vars, node->frame.cfa);

    for (int i = 0; i < node->markers.pos; ++i) {
        Marker* child_marker= Marker_vec_get_unsafe(&node->markers, i);

        if (is_hittable_type(child_marker->pos->type)) {
            const DEP_HITTABLE* hittable= (DEP_HITTABLE*)child_marker->pos;

            const uintptr_t r_addr= target.target_addr_virtual_to_runtime(hittable->addr);
            target.target_remove_bp_at_addr_cfa(r_addr, BP_REASON_BREAK_CAUSE, node->cfa);
            if (child_marker->going_upstream) {
                target.target_remove_bp_at_addr_cfa(r_addr + hittable->value_addr_offset, BP_REASON_BREAK_CAUSE, node->cfa);
            }
        }

        if (child_marker->pos->type == DEPTYPE_HEADER) {
            DEP_HEADER* header= (DEP_HEADER*)child_marker->pos;
            const uintptr_t r_addr= target.target_addr_virtual_to_runtime(header->addr);
            target.target_remove_bp_at_addr_cfa(r_addr, BP_REASON_BREAK_CAUSE, node->cfa);
        }
    }

    const SubInfo* sub_info= find_sub_info(header->addr);
    if (sub_info) {
        const uintptr_t* end_addr= sub_info->func_ends;
        while (*end_addr != FUNC_ENDS_END) {
            const uintptr_t r_addr= target.target_addr_virtual_to_runtime(*end_addr);
            target.target_remove_bp_at_addr_cfa(r_addr, BP_REASON_BREAK_CAUSE, node->cfa);
            end_addr += 1;
        }
    }


    vector_disseminate_destruction((Vector*)&marker->node->markers);
}

static void place_marker_on(DEP_BASE* base, Marker* existing_marker, TargetValue target_value);

CollectInfo* get_or_add_collect_info(DEP_COLLECT* collect, TreeNode* node) {
    CollectInfo* info= CollectInfo_arr_search_ie(&node->collections, collect);
    if (!info) {
        CollectInfo_arr_add(&node->collections, (CollectInfo) {
            .collect= collect,
            .target_value= TARGET_VALUE_BASE,
            .current_hits= 0
        });
        return CollectInfo_arr_peek(&node->collections);
    }
    return info;
}

TargetRule rule_from_target_value(TargetValue* value) {
    switch (value->type) {
        case TARGET_VALUE_LOGICAL: {
            return (TargetRule) {
                .op= value->inverted ? OP_EQ : OP_NE,
                .value= 0
            };
        }
        case TARGET_VALUE_VALUE: {
            return (TargetRule) {
                .op= value->inverted ? OP_NE : OP_EQ,
                .value= value->data.value
            };
        }

        case TARGET_VALUE_RULES:
        case TARGET_VALUE_RANGE:
        case TARGET_VALUE_ANY:
        default:
            assert(false);
    }
}

void add_to_target_value(TargetValue* dst, TargetValue src) {
    if (dst->type == TARGET_VALUE_ANY) {
        if (dst->inverted) return;

        *dst= src;
        return;
    }

    if (src.type == TARGET_VALUE_ANY) {
        dst->type= TARGET_VALUE_ANY;
        dst->inverted= src.inverted;
        return;
    }

    dst->inverted= false;
    if (dst->type != TARGET_VALUE_RULES) {
        const TargetRule dst_rule= rule_from_target_value(dst);
        dst->type= TARGET_VALUE_RULES;
        dst->data.rules= TargetRule_arr_construct(1);
        TargetRule_arr_add(&dst->data.rules, dst_rule);
    }

    if (src.type == TARGET_VALUE_RULES) {
        for (int i = 0; i < src.data.rules.pos; ++i) {
            TargetRule rule= TargetRule_arr_get(&src.data.rules, i);
            TargetRule_arr_add(&dst->data.rules, rule);
        }
        return;
    }

    TargetRule_arr_add(&dst->data.rules, rule_from_target_value(&src));
}

void collect_triggered_callback(DEP_COLLECT* collect, Marker* marker, TargetValue target_value) {
    show_log("DEP Collect triggered\n");
    marker->target_value= target_value;
    place_marker_on(collect->link, marker, target_value);
}

Marker* create_marker_for(DEP_BASE* base, Marker* existing_marker, TargetValue target_value, bool dead) {
    Marker* m= create_marker(
        existing_marker,
        base->dead || (existing_marker && existing_marker->dead) || dead,
        base,
        target_value
    );

    if (!markers.arr) markers= Marker_vec_create();
    Marker_vec_add(&markers, m);

    bool succ;
    const uintptr_t cfa= target.target_get_cfa(&succ);

    if (!existing_marker) {
        TreeNode* new_node= alloc_node(cfa, base);
        root= new_node;
        m->node= new_node;
    } else {
        m->node= existing_marker->node;
    }

    if (base->type != DEPTYPE_HEADER)
        Marker_vec_add(&m->node->markers, m);

    m->target_value= target_value;

    if (is_hittable_type(base->type)) {
        const DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;
        m->pc= hittable->addr;
    }

    if (base->type == DEPTYPE_COLLECT && existing_marker) {
        CollectInfo* info= get_or_add_collect_info((DEP_COLLECT*)base, existing_marker->node);

        info->current_hits++;
        add_to_target_value(&info->target_value, target_value);

        if (info->current_hits >= info->collect->conns) {
            collect_triggered_callback((DEP_COLLECT*)base, m, info->target_value);
        }
    }

    return m;
}

void place_or_hit_downstream_callback(uintptr_t r_addr, uintptr_t cfa, Marker* marker) {
    if (marker->pos->type == DEPTYPE_COLLECT) {
        return;
    }

    if (target.target_get_pc() == r_addr) {
        marker_hit_downstream_callback(marker);
        return;
    }

    target.target_place_bp_with_cfa(r_addr, cfa, BP_REASON_BREAK_CAUSE, marker_hit_downstream_callback, marker);
}

void place_marker_on(DEP_BASE* base, Marker* existing_marker, const TargetValue target_value) {
    Marker* m= create_marker_for(base, existing_marker, target_value, base->dead);

    printf("Created marker for %s", DEPTYPE_STRS[base->type]);
    if (is_hittable_type(base->type)) printf(" @ %#lx", ((DEP_HITTABLE*)base)->addr);
    newline();

    if (base->pass) {
        marker_hit_downstream_callback(m);
        return;
    }

    bool succ;
    uintptr_t cfa= target.target_get_cfa(&succ);
    if (is_hittable_type(base->type)) {
        const DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;

        // todo add const check

        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(hittable->addr);
        place_or_hit_downstream_callback(r_addr, cfa, m);
        printf("Placed bp @ %#lx with cfa %#lx\n", r_addr, cfa);
    }

    if (base->type == DEPTYPE_HEADER) {
        /*
         * If this is a header then we need to place points on the end of the function
         */
        const DEP_HEADER* header= (DEP_HEADER*)base;

        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(header->addr);
        place_or_hit_downstream_callback(r_addr, -1, m);
    }
}

void place_initial_marker(DEP_BASE* base, uint64_t target_value) {
    place_marker_on(base, NULL, TARGET_CONST(target_value));
}

int break_on_cause(bool is_simple) {
    // assume the ident and line are 'res' and 34 resp.
    // this is creating the initial setup
    DEP_BASE* first;
    int64_t target_value;
    if (is_simple) {
        first= (DEP_BASE*)&main_res_34;
        sub_infos= simple_sub_infos;
        target_value= 1;
    }
    else {
        first= (DEP_BASE*)&parse_local_res;
        sub_infos= example_sub_infos;
        target_value= FAIL;
    }

    place_initial_marker(first, target_value);

    return SUCCESS;
}

size_t split_link_count(DEP_SPLIT* split) {
    DEP_BASE* last= split->links[0];

    size_t i= 0;
    while (last->type != DEPTYPE_SENTINEL) {
        i++;
    }

    return i;
}

void cleanup_marker(Marker* marker) {
    target.target_remove_bp_at_addr(marker->pc, BP_REASON_BREAK_CAUSE);

    free(marker);
}

bool matches_rule(TargetRule* rule, int64_t value) {
    switch (rule->op) {
        case OP_EQ: return value == rule->value;
        case OP_NE: return value != rule->value;
        case OP_GT: return value > rule->value;
    }
    assert(false);
}

bool is_target_value_reached(const TargetValue target_val, DEP_BASE* base) {
    assert(is_hittable_type(base->type));
    Location result_loc;

    if (base->type == DEPTYPE_EXPR) {
        const DEP_EXPR* expr= (DEP_EXPR*)base;
        result_loc= expr->result_loc;
    } else if (base->type == DEPTYPE_LINEAR) {
        const DEP_LINEAR* lin= (DEP_LINEAR*)base;
        result_loc= lin->result_loc;
    } else assert(false);

    const int64_t value= get_location_data(result_loc);

    switch (target_val.type) {
        case TARGET_VALUE_LOGICAL: {
            if (target_val.inverted) return value == 0;
            return value != 0;
        }
        case TARGET_VALUE_VALUE: {
            if (target_val.inverted) return value != target_val.data.value;
            return value == target_val.data.value;
        }
        case TARGET_VALUE_RULES: {
            bool matches_all= true;
            for (int i = 0; i < target_val.data.rules.pos; ++i) {
                TargetRule* rule= TargetRule_arr_ptr(&target_val.data.rules, i);
                matches_all= matches_all && matches_rule(rule, value);
                if (!matches_all) break;
            }

            return matches_all;
        }
        case TARGET_VALUE_RANGE: {
            const bool in_range= value <= target_val.data.range.end && value >= target_val.data.range.start;
            if (target_val.inverted) return !in_range;
            return in_range;
        }
        case TARGET_VALUE_ANY: return true;
        default: assert(false);
    }
}

void print_marker_links(const Marker* marker) {
    if (!marker->prev) return;

    printf(" -> ");
    printf("%u", marker->prev->pos->id);
    print_marker_links(marker->prev);
}

void print_target_value(TargetValue* target_value) {
    if (target_value->inverted) {
        printf("NOT: ");
    }
    switch (target_value->type) {
        case TARGET_VALUE_LOGICAL: {
            printf("true");
            break;
        }
            break;
        case TARGET_VALUE_VALUE: {
            printf("%ld", target_value->data.value);
            break;
        }
        case TARGET_VALUE_RULES: {
            for (int i = 0; i < target_value->data.rules.pos; ++i) {
                TargetRule* rule= TargetRule_arr_ptr(&target_value->data.rules, i);
                printf("Rule: val %s %ld ", OPERATOR_STRS[rule->op], rule->value);
            }
            break;
        }
        case TARGET_VALUE_RANGE: {
            printf("RANGE INCLUSIVE %ld-%ld", target_value->data.range.start, target_value->data.range.end);
            break;
        }
        case TARGET_VALUE_ANY: {
            printf("ANY");
            break;
        }
    }
}

void print_node(TreeNode* node, uint8_t depth) {
    const uintptr_t r_addr= node->frame.pc;
    const uintptr_t v_addr= node->frame.v_pc;

    printf("\n--[%u]--\n", depth);
    printf("Stack frame for %s @ %#lx (%#lx) cfa %#lx\n", node->frame.sub->subprog_name, r_addr, v_addr, node->frame.cfa);
    printf("There are %lu markers:\n", node->markers.pos);
    for (int i = 0; i < node->markers.pos; i++) {
        const Marker* marker= Marker_vec_get_unsafe(&node->markers, i);
        printf("\tMarker (dep id: %u) @ %#lx target value: ", marker->pos->id, marker->pc);
        print_target_value(&marker->target_value);
        print_marker_links(marker);
        newline();
    }
    newline();

    if (node->links.pos != 0) {
        printf("Linked nodes:\n");
        for (int i = 0; i < node->links.pos; i++) {
            print_node(Node_vec_get_unsafe(&node->links, i), depth + 1);
        }
    }
}

void print_root() {
    printf("STACK ROOT: \n");
    print_node(root, 0);
}

void handle_marker_hit_upstream(Marker* marker) {
    printf("hit marker upstream %#lx\n", marker->pc);
    if (marker->dead) {
        printf("Hit dead marker\n");
        return;
    }

    if (is_target_value_reached(marker->target_value, marker->pos)) {
        show_log("break cause hit! by marker: %u\n", marker->pos->id);
        g_idle_add(display_break_cause_tree, root);
        change_state(STATE_NORMAL);
    }
}

uintptr_t addr_of(DEP_BASE* base) {
    if (is_hittable_type(base->type)) {
        return ((DEP_HITTABLE*)base)->addr;
    }

    switch (base->type) {
        case DEPTYPE_SPLIT: {
            DEP_SPLIT* split= (DEP_SPLIT*)base;
            return addr_of(split->links[0]);
        }
        case DEPTYPE_COND: {
            const DEP_COND* cond= (DEP_COND*)base;
            return addr_of(cond->condition);
        }
        case DEPTYPE_HEADER: {
            const DEP_HEADER* hdr= (DEP_HEADER*)base;
            return addr_of(hdr->link);
        }
        default: assert(false);
    }
}

uintptr_t result_addr_of(DEP_BASE* base) {
    if (is_hittable_type(base->type)) {
        DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;
        return hittable->addr + hittable->value_addr_offset;
    }

    switch (base->type) {
        case DEPTYPE_SPLIT: {
            DEP_SPLIT* split= (DEP_SPLIT*)base;
            return result_addr_of(split->links[0]);
        }
        case DEPTYPE_COND: {
            const DEP_COND* cond= (DEP_COND*)base;
            return result_addr_of(cond->condition);
        }
        case DEPTYPE_HEADER: {
            const DEP_HEADER* hdr= (DEP_HEADER*)base;
            return result_addr_of(hdr->link);
        }
        default: assert(false);
    }
}

static bool place_next_markers(Marker* marker, DEP_BASE* base, TargetValue target);

bool place_header_marker(Marker* marker, DEP_HEADER* header, TargetValue target_value) {
    if (header->link == NULL) assert(false);

    place_marker_on(header->link, marker, target_value);
    return true;
}

bool place_linear_marker(Marker* marker, DEP_LINEAR* lin, TargetValue target_value) {
    if (lin->link == NULL) return false;

    if (addr_of(lin->link) == lin->addr) {
        return place_next_markers(marker, lin->link, target_value);
    }

    place_marker_on(lin->link, marker, target_value);

    return true;
}

TargetValue to_target_value(const int64_t value) {
    return TARGET_CONST(value);
}

int64_t get_value(DEP_BASE* base) {
    if (!is_hittable_type(base->type)) assert(false);

    const DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;

    return get_location_data(hittable->result_loc);
}

void expr_left_evaluated_callback(void* data) {
    Marker* marker= data;

    // the left side of the expression has now been evaluated
    //  so need to place the right marker with the new target value
    const DEP_EXPR* expr= (DEP_EXPR*)marker;

    const int64_t value= get_value(expr->left);
    const TargetValue target_val= to_target_value(value);
    place_next_markers(marker, expr->right, target_val);
}

int64_t get_location_data(const Location loc) {
    int64_t value;
    switch (loc.type) {
        case LOCATION_REGISTER:
            value= target.target_get_reg(loc.data.register_id).value.general;
            break;
        case LOCATION_REGISTER_OFFSET: {
            const int64_t reg_value= target.target_get_reg(loc.data.reg_off.register_id).value.general;
            value= target.target_get_general_reg_at(reg_value + loc.data.reg_off.offset);
            break;
        }
        case LOCATION_COMPARE:
            return target.target_check_comparison(loc.data.comparison);
        case LOCATION_CONST: {
            return loc.data.const_val;
        }
        default: assert(false);
    }
    if (loc.byte_size == 0 || loc.byte_size == 8) return value;

    switch (loc.byte_size) {
        case 4: return value & 0xFFFFFFFF; break;
        case 2: return value & 0xFFFF; break;
        case 1: return value & 0xFF; break;
        default: assert(false);
    }
}

bool place_expr_markers(Marker* marker, DEP_EXPR* expr, TargetValue target_value) {
    bool succ;
    const uintptr_t cfa= target.target_get_cfa(&succ);

    TargetValue const_val= TARGET_VALUE_BASE;

    Marker* left_marker= NULL;
    Marker* right_marker= NULL;

    const bool left_alive= expr->type == TYPE_LEFT && !marker->dead;
    const bool right_alive= expr->type == TYPE_RIGHT || expr->type == TYPE_BOTH && !marker->dead;

    switch (expr->type) {
        case TYPE_LEFT: {
            const DEP_LINEAR* lin= (DEP_LINEAR*)expr->right;
            const int64_t right_val= get_location_data(lin->result_loc);
            const_val= TARGET_CONST(right_val);

            break;
        }
        case TYPE_RIGHT: {
            const DEP_LINEAR* lin= (DEP_LINEAR*)expr->left;
            const int64_t left_val= get_location_data(lin->result_loc);
            const_val= TARGET_CONST(left_val);

            break;
        }
        case TYPE_BOTH: {
            marker->placed_right_expr_marker= false;
            const uintptr_t r_addr= result_addr_of(expr->left);
            target.target_place_bp_with_cfa(
                r_addr,
                cfa,
                BP_REASON_BREAK_CAUSE,
                expr_left_evaluated_callback,
                marker
            );
        }
    }

    if (expr->type == TYPE_NONE) {
        return false;
    }

    switch (expr->op) {
        case OP_EQ: const_val.inverted= false;
            break;
        case OP_NE: const_val.inverted= true;
            break;
        case OP_GT: {
            const_val.inverted= false;
            int64_t gt_value= const_val.data.value;
            const_val= (TargetValue) {
                .type= TARGET_VALUE_RULES,
                .data.rules= TargetRule_arr_construct(1)
            };
            TargetRule_arr_add(&const_val.data.rules, (TargetRule) {
                .op= OP_GT,
                .value= gt_value
            });
            break;
        }
        default:
            assert(false);
    }

    left_marker= create_marker_for(expr->left, marker, const_val, !left_alive);
    right_marker= create_marker_for(expr->right, marker, const_val, !right_alive);

    if (left_marker->pc != 0) {
        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(left_marker->pc);
        place_or_hit_downstream_callback(r_addr, cfa, left_marker);
    }

    if (right_marker->pc != 0) {
        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(right_marker->pc);
        place_or_hit_downstream_callback(r_addr, cfa, right_marker);
    }

    return true;
}

bool place_next_markers(Marker* marker, DEP_BASE* base, TargetValue target_value) {
    switch (base->type) {
        case DEPTYPE_LINEAR: return place_linear_marker(marker, (DEP_LINEAR*)base, target_value);
        case DEPTYPE_EXPR: return place_expr_markers(marker, (DEP_EXPR*)base, target_value);
        case DEPTYPE_HEADER: return place_header_marker(marker, (DEP_HEADER*)base, target_value);
        default: assert(false);
    }
}

void handle_marker_hit_downstream(Marker* marker) {
    bool succ;
    const uintptr_t cfa= target.target_get_cfa(&succ);

    if (marker->pos->type == DEPTYPE_HEADER) {
        const DEP_HEADER* header= (DEP_HEADER*)marker->pos;
        const SubInfo* sub_info= find_sub_info(header->addr);

        TreeNode* node= alloc_node(cfa, marker->pos);
        Node_vec_add(&marker->node->links, node);
        marker->node= node;//todo: add marker to node
        Marker_vec_add(&node->markers, marker);

        size_t idx= 0;
        while (sub_info->func_ends[idx] != FUNC_ENDS_END) {
            const uintptr_t r_addr= target.target_addr_virtual_to_runtime(sub_info->func_ends[idx]);
            target.target_place_bp_defered(r_addr, cfa, BP_REASON_BREAK_CAUSE, func_end_callback, marker);
            idx++;
        }

        place_next_markers(marker, marker->pos, marker->target_value);
        return;
    }

    if (marker->pos->type == DEPTYPE_SPLIT) {
        const DEP_SPLIT* split= (DEP_SPLIT*)marker->pos;
        size_t i= 0;
        while (split->links[i]->type != DEPTYPE_SENTINEL) {
            place_marker_on(split->links[i], marker, marker->target_value);
            i++;
        }
        return;
    }

    if (marker->pos->type == DEPTYPE_COND) {
        const DEP_COND* cond= (DEP_COND*)marker->pos;

        place_marker_on(cond->condition, marker, TARGET_LOGICAL(true));
        place_marker_on(cond->link, marker, marker->target_value);
        return;
    }

    printf("hit marker downstream %#lx\n", marker->pc);
    if (!is_hittable_type(marker->pos->type)) {
        show_err("Hit non-hittable (non-linear, non-expr) marker (%s), should not occur\n", DEPTYPE_STRS[marker->pos->type]);
        return;
    }

    const DEP_HITTABLE* hit= (DEP_HITTABLE*)marker->pos;

    if (hit->value_addr_offset != 0) {
        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(hit->addr);
        const uintptr_t res_addr= r_addr + hit->value_addr_offset;
        target.target_place_bp_with_cfa(
            res_addr,
            cfa,
            BP_REASON_BREAK_CAUSE,
            marker_hit_upstream_callback,
            marker
        );
    }

    const bool placed= place_next_markers(marker, marker->pos, marker->target_value);
    // if (!placed) {
    //     propogate_marker_up(marker);
    // }
}

void marker_hit_downstream_callback(void* data) {
    Marker* const marker= data;
    marker->going_upstream= true;
    handle_marker_hit_downstream(marker);
}

void marker_hit_upstream_callback(void* data) {
    Marker* const marker= data;
    handle_marker_hit_upstream(marker);
}


