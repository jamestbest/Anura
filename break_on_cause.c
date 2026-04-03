//
// Created by jamestbest on 3/10/26.
//

#include "break_on_cause.h"

#include <stdint.h>

#include "Vector.h"
#include "Array.h"
#include "Errors.h"
#include "Target.h"
#include "Palantir/Palantir.h"

static void marker_hit_downstream_callback(void* data);
static void marker_hit_upstream_callback(void* data);

const char* DEPTYPE_STRS[DEPTYPE_COUNT]= {
    [DEPTYPE_LINEAR]= "DEPTYPE_LINEAR",
    [DEPTYPE_SPLIT]= "DEPTYPE_SPLIT",
    [DEPTYPE_COND]= "DEPTYPE_COND",
    [DEPTYPE_SENTINEL]= "DEPTYPE_SENTINEL",
    [DEPTYPE_HEADER]= "DEPTYPE_HEADER",
    [DEPTYPE_EXPR]= "DEPTYPE_EXPR"
};

DEP_BASE SENTINEL= (DEP_BASE) {.type= DEPTYPE_SENTINEL, .dead=false};

#define NO_PASS false
#define PASS true

#define NOT_DEAD false
#define IS_DEAD true

#define NOT_LEFT false
#define IS_LEFT true

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

DEP_LINEAR res_set_as_fail= MAKE_DATA(16, LOC_REG_OFF(RBP_CODE, -0x4), 0x11a9);
DEP_COND main_res_34= MAKE_COND(17, NOT_DEAD, IS_LEFT, t1_fail_expr, res_set_as_fail);




ARRAY_ADD(uintptr_t, Addr)

const SubInfo main_info= (SubInfo) {
    .func_start= 0x1180,
    .func_ends= {
        0x11ac,
        FUNC_ENDS_END
    }
};

const SubInfo t1_info= (SubInfo) {
    .func_start= 0x1147,
    .func_ends= {
        0x117e,
        FUNC_ENDS_END
    }
};

const SubInfo t2_info= (SubInfo) {
    .func_start= 0x1138,
    .func_ends= {
        0x1145,
        FUNC_ENDS_END
    }
};

const SubInfo t3_info= (SubInfo) {
    .func_start= 0x1129,
    .func_ends= {
        0x1136,
        FUNC_ENDS_END
    }
};

const SubInfo* sub_infos[]= {
    &main_info,
    &t1_info,
    &t2_info,
    &t3_info,
    NULL
};


#define ID(num) num

DEP_LINEAR global_tidx= MAKE_DATA(
    ID(100),
    LOC_REG_OFF(RIP_CODE, 0x2c95),
    0x142c
)

DEP_LINEAR global_max_tidx= MAKE_DATA(
    ID(101),
    LOC_REG_OFF(RIP_CODE, 0x2c96),
    0x1433
);

DEP_EXPR current_expr= MAKE_EXPR(
    ID(4),
    NOT_DEAD,
    NO_PASS,
    OP_GT,
    global_tidx,
    global_max_tidx,
    TYPE_LEFT,
    0x142c,
    17,
    LOC_COMPARE(COMPARE_GT)
);

DEP_LINEAR current_zero= MAKE_CONST(
    ID(3),
    0,
    0x143f
)

DEP_COND current_cond= MAKE_COND(
    ID(2),
    NOT_DEAD,
    IS_LEFT,
    current_expr,
    current_zero
);

DEP_LINEAR current_return= MAKE_DATA(
    ID(5),
    LOC_RAX,
    0x145e
);

DEP_SPLIT current_split= MAKE_SPLIT(
    ID(1),
    NOT_DEAD, NO_PASS,
    (DEP_BASE*)&current_cond,
    (DEP_BASE*)&current_return
);

DEP_HEADER current_header= MAKE_HEADER(
    ID(0),
    0x1424,
    current_split,
    NOT_DEAD,
    NO_PASS
);





DEP_LINEAR consume_return= MAKE_DATA(
    ID(11),
    LOC_RAX,
    0x1422
);

DEP_LINEAR consume_zero= MAKE_CONST(
    ID(10),
    0,
    0x13f8
);

DEP_EXPR consume_expr= MAKE_EXPR(
    ID(9),
    NOT_DEAD,
    NO_PASS,
    OP_GT,
    global_tidx,
    global_max_tidx,
    TYPE_LEFT,
    0x13e5,
    17,
    LOC_COMPARE(COMPARE_GT)
);

DEP_COND consume_cond= MAKE_COND(
    ID(8),
    NOT_DEAD,
    IS_LEFT,
    consume_expr,
    consume_zero
);

DEP_SPLIT consume_split= MAKE_SPLIT(
    ID(7),
    NOT_DEAD,
    NO_PASS,
    (DEP_BASE*)&consume_cond,
    (DEP_BASE*)&consume_return
);

DEP_HEADER consume_header= MAKE_HEADER(
    ID(6),
    0x13dd,
    consume_split,
    NOT_DEAD,
    NO_PASS
);




DEP_LINEAR error_return= MAKE_CONST(
    ID(13),
    FAIL,
    0x1387
);

DEP_HEADER error_header= MAKE_HEADER(
    ID(12),
    0x137b,
    error_return,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR expect_consume= MAKE_SINGLE(
    ID(16),
    0x13d6,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    consume_header,
    "consume()"
);

DEP_LINEAR expect_expr_const_zero= MAKE_CONST(
    ID(19),
    0,
    0x13ab
);

DEP_LINEAR expect_local_c= MAKE_SINGLE(
    ID(20),
    0x13a2,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    current_header,
    "Token* c= current()"
);

DEP_EXPR expect_expr_not_c= MAKE_EXPR(
    ID(18),
    NOT_DEAD,
    PASS,
    OP_EQ,
    expect_local_c,
    expect_expr_const_zero,
    TYPE_LEFT,
    0x13a7,
    9,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR expect_first_zero= MAKE_CONST(
    ID(21),
    0,
    0x13b2
);

DEP_COND expect_null_check= MAKE_COND(
    ID(17),
    NOT_DEAD,
    true,
    expect_expr_not_c,
    expect_first_zero
);

DEP_LINEAR expect_second_zero= MAKE_CONST(
    ID(23),
    0,
    0x13ca
);

DEP_LINEAR expect_expr_param_type= MAKE_DATA(
    ID(24),
    LOC_REG_OFF(RBP_CODE, -0x14),
    0x13c5
);

DEP_LINEAR expect_current_type= MAKE_SINGLE(
    ID(26),
    0x13be,
    7,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    current_header,
    "current()->type"
);

DEP_EXPR expect_expr_current= MAKE_EXPR(
    ID(25),
    NOT_DEAD,
    PASS,
    OP_NE,
    expect_current_type,
    expect_expr_param_type,
    TYPE_LEFT,
    0x13be,
    10,
    LOC_COMPARE(COMPARE_NEQ)
);

DEP_COND expect_type= MAKE_COND(
    ID(22),
    NOT_DEAD,
    IS_LEFT,
    expect_expr_current,
    expect_second_zero
);

DEP_SPLIT expect_split= MAKE_SPLIT(
    ID(15),
    NOT_DEAD,
    (DEP_BASE*)&expect_consume,
    (DEP_BASE*)&expect_type,
    (DEP_BASE*)&expect_null_check
);


DEP_HEADER expect_header= MAKE_HEADER(
    ID(14),
    0x138e,
    expect_split,
    NOT_DEAD,
    NO_PASS
);










VECTOR_ADD(Marker, Marker)
VECTOR_ADD(struct TreeNode, Node)

MarkerVector markers;
TreeNode* root;

TreeNode* alloc_node(uint64_t cfa, DEP_BASE* base) {
    TreeNode* node= malloc(sizeof(TreeNode));
    const uintptr_t pc= target.target_get_pc();
    const uintptr_t v_pc= target.target_addr_runtime_to_virtual(pc);
    const char* sub_name= target.target_get_subroutine_name_at(v_pc);

    *node= (TreeNode) {
        .cfa= cfa,
        .links= Node_vec_create(),
        .base= base,
        .frame= (StackFrame){
            .cfa= cfa,
            .pc= pc,
            .v_pc= v_pc,
            .subprog_name= sub_name
        },
        .markers= Marker_vec_create(),
    };

    return node;
}

bool target_to_logical(const TargetValue target) {
    if (target.is_logical) return target.logical;

    return target.value != 0;
}

#define TARGET_LOGICAL(val) (TargetValue) {.is_logical= true, .logical= val}
#define TARGET_CONST(val) (TargetValue) {.is_logical= false, .value= val}

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

void func_end_callback(void* data) {
    Marker* marker= data;
    const DEP_HEADER* header= (DEP_HEADER*)marker->pos;

    TreeNode* node= marker->node;
    node->frame.end_stack_pointer= target.target_get_reg(RSP_CODE).value.general;

    const size_t size= node->frame.cfa - node->frame.end_stack_pointer + 1;
    node->frame.data= target.target_get_data_runtime(node->frame.cfa, size).raw_data;

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

static void place_marker_on(DEP_BASE* base, Marker* existing_marker, const TargetValue target_value);

Marker* create_marker_for(DEP_BASE* base, Marker* existing_marker, TargetValue target_value, bool dead) {
    Marker* m= create_marker(
        existing_marker,
        base->dead || (existing_marker && existing_marker->dead),
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

    switch (base->type) {
        case DEPTYPE_LINEAR:
        case DEPTYPE_EXPR: {
            const DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;
            m->pc= hittable->addr;
            m->target_value= target_value;
            break;
        }

        case DEPTYPE_SPLIT: {
            DEP_SPLIT* split= (DEP_SPLIT*)base;
            size_t i= 0;
            while (split->links[i]->type != DEPTYPE_SENTINEL) {
                place_marker_on(split->links[i], m, target_value);
                i++;
            }
            break;
        }

        case DEPTYPE_COND: {
            const DEP_COND* cond= (DEP_COND*)base;

            place_marker_on(cond->condition, m, TARGET_LOGICAL(true));
            place_marker_on(cond->link, m, target_value);
            break;
        }
        case DEPTYPE_HEADER: {
            break;
        }
        case DEPTYPE_SENTINEL:
            assert(false);
    }

    return m;
}

void place_marker_on(DEP_BASE* base, Marker* existing_marker, const TargetValue target_value) {
    Marker* m= create_marker_for(base, existing_marker, target_value, base->dead);

    printf("Created marker for %s", DEPTYPE_STRS[base->type]);
    if (is_hittable_type(base->type)) printf(" @ %#lx", ((DEP_HITTABLE*)base)->addr);
    newline();

    bool succ;
    uintptr_t cfa= target.target_get_cfa(&succ);
    if (is_hittable_type(base->type)) {
        const DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;

        // todo add const check

        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(hittable->addr);
        target.target_place_bp_with_cfa(r_addr, cfa, BP_REASON_BREAK_CAUSE, marker_hit_downstream_callback, m);
        printf("Placed bp @ %#lx with cfa %#lx\n", r_addr, cfa);
    }

    if (base->type == DEPTYPE_HEADER) {
        /*
         * If this is a header then we need to place points on the end of the function
         */
        const DEP_HEADER* header= (DEP_HEADER*)base;

        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(header->addr);
        target.target_place_bp_with_cfa(r_addr, -1, BP_REASON_BREAK_CAUSE, marker_hit_downstream_callback, m);
    }
}

void place_initial_marker(DEP_BASE* base, uint64_t target_value) {
    place_marker_on(base, NULL, TARGET_CONST(target_value));
}

int break_on_cause(const char* ident, uint32_t line) {
    // assume the ident and line are 'res' and 34 resp.
    // this is creating the initial setup
    DEP_BASE* first= (DEP_BASE*)&main_res_34;

    bool succ;
    const uint64_t cfa= target.target_get_cfa(&succ);
    if (!succ) return FAIL;

    // root= alloc_node(cfa, first);

    place_initial_marker(first, FAIL);

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

bool target_value_reached(TargetValue target_val, DEP_BASE* base) {
    assert(is_hittable_type(base->type));

    const DEP_LINEAR* lin= (DEP_LINEAR*)base;
    //
    int64_t value;
    switch (lin->result_loc.type) {
        case LOCATION_REGISTER:
            value= target.target_get_reg(lin->result_loc.data.register_id).value.general;
            break;
        case LOCATION_REGISTER_OFFSET: {
            int64_t reg_value= target.target_get_reg(lin->result_loc.data.reg_off.register_id).value.general;
            value= target.target_get_general_reg_at(reg_value + lin->result_loc.data.reg_off.offset);
            break;
        }
        case LOCATION_COMPARE:
            value= target.target_check_comparison(lin->result_loc.data.comparison);
            break;
        case LOCATION_CONST: {
            value= lin->result_loc.data.const_val;
            break;
        }
        default: assert(false);
    }


    if (target_val.is_logical) {
        if (target_val.logical) {
            return value != 0;
        }
        return value == 0;
    }

    return value == target_val.value;
}

void propogate_marker_up(const Marker* marker) {
    if (marker->dead) {
        return;
    }

    if (target_value_reached(marker->target_value, marker->pos)) {
        g_idle_add(display_break_cause_tree, root);
    }
}

void print_marker_links(const Marker* marker) {
    if (!marker->prev) return;

    printf(" -> ");
    printf("%u", marker->prev->pos->id);
    print_marker_links(marker->prev);
}

void print_node(TreeNode* node, uint8_t depth) {
    const uintptr_t r_addr= node->frame.pc;
    const uintptr_t v_addr= node->frame.v_pc;

    printf("\n--[%u]--\n", depth);
    printf("Stack frame for %s @ %#lx (%#lx) cfa %#lx\n", node->frame.subprog_name, r_addr, v_addr, node->frame.cfa);
    printf("There are %lu markers:\n", node->markers.pos);
    for (int i = 0; i < node->markers.pos; i++) {
        const Marker* marker= Marker_vec_get_unsafe(&node->markers, i);
        printf("\tMarker (dep id: %u) @ %#lx target value: %ld", marker->pos->id, marker->pc, marker->target_value.value);
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

    if (target_value_reached(marker->target_value, marker->pos)) {
        show_log("break cause hit!\n");
        g_idle_add(display_break_cause_tree, root);
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

TargetValue to_target_value(int64_t value) {
    return (TargetValue) {
        .value= value,
        .is_logical= false
    };
}

int64_t get_value(DEP_BASE* base) {
    if (!is_hittable_type(base->type)) assert(false);

    DEP_HITTABLE* hittable= (DEP_HITTABLE*)base;

    switch (hittable->result_loc.type) {
        case LOCATION_REGISTER: {
            return target.target_get_reg(hittable->result_loc.data.register_id).value.general;
        }
        case LOCATION_REGISTER_OFFSET: {
            const int64_t reg_val= target.target_get_reg(hittable->result_loc.data.reg_off.register_id).value.general;
            return target.target_get_general_reg_at(reg_val + hittable->result_loc.data.reg_off.offset);
        }
        case LOCATION_COMPARE: {
            return target.target_check_comparison(hittable->result_loc.data.comparison);;
        }
        case LOCATION_CONST: {
            return hittable->result_loc.data.const_val;
        }
    }

    assert(false);
}

void expr_left_evaluated_callback(void* data) {
    Marker* marker= (Marker*)data;

    // the left side of the expression has now been evaluated
    //  so need to place the right marker with the new target value
    const DEP_EXPR* expr= (DEP_EXPR*)marker;

    const int64_t value= get_value(expr->left);
    const TargetValue target_val= to_target_value(value);
    place_next_markers(marker, expr->right, target_val);
}


bool place_expr_markers(Marker* marker, DEP_EXPR* expr, TargetValue target_value) {
    bool succ;
    const uintptr_t cfa= target.target_get_cfa(&succ);

    TargetValue const_val= (TargetValue) {
        .value= -1,
        .is_logical= false
    };

    Marker* left_marker= NULL;
    Marker* right_marker= NULL;

    bool left_alive= expr->type == TYPE_LEFT && !marker->dead;
    bool right_alive= expr->type == TYPE_RIGHT || expr->type == TYPE_BOTH && !marker->dead;

    switch (expr->type) {
        case TYPE_LEFT: {
            const DEP_LINEAR* lin= (DEP_LINEAR*)expr->right;
            const_val.value= lin->result_loc.data.const_val;

            break;
        }
        case TYPE_RIGHT: {
            const DEP_LINEAR* lin= (DEP_LINEAR*)expr->left;
            const_val.value= lin->result_loc.data.const_val;

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

    left_marker= create_marker_for(expr->left, marker, const_val, !left_alive);
    right_marker= create_marker_for(expr->right, marker, const_val, !right_alive);

    if (addr_of(expr->left) == expr->addr) {
        marker_hit_downstream_callback(left_marker);
    } else {
        uintptr_t r_addr= target.target_addr_virtual_to_runtime(left_marker->pc);
        target.target_place_bp_with_cfa(
            r_addr,
            cfa,
            BP_REASON_BREAK_CAUSE,
            marker_hit_downstream_callback,
            left_marker
        );
    }

    if (addr_of(expr->right) == expr->addr) {
        marker_hit_downstream_callback(right_marker);
    } else {
        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(right_marker->pc);
        target.target_place_bp_with_cfa(
            r_addr,
            cfa,
            BP_REASON_BREAK_CAUSE,
            marker_hit_downstream_callback,
            right_marker
        );
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
            target.target_place_bp_with_cfa(r_addr, cfa, BP_REASON_BREAK_CAUSE, func_end_callback, marker);
            idx++;
        }

        place_next_markers(marker, marker->pos, marker->target_value);
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
    if (!placed) {
        propogate_marker_up(marker);
    }
}

void marker_hit_downstream_callback(void* data) {
    Marker* const marker= data;
    handle_marker_hit_downstream(marker);
}

void marker_hit_upstream_callback(void* data) {
    Marker* const marker= data;
    handle_marker_hit_upstream(marker);
}


