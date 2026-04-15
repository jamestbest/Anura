//
// Created by jamestbest on 4/7/26.
//

#include "break_save.h"

#include "IsildursBane.h"

#define RAX_CODE 0
#define RBP_CODE 6

#define MAKE_CF(addr_, reason_) {\
    .base= (POINT){.type= POINT_TYPE_CF, .addr= addr_},\
    .reason= reason_, \
    .custom_reason= NULL\
};

#define MAKE_DF_LOC(addr_, reason_, die_offset_, loc_) {\
    .base= (POINT){.type= POINT_TYPE_DF, .addr= addr_},\
    .reason= reason_,\
    .custom_reason= NULL,\
    .die_offset= die_offset_,\
    .loc= loc_\
};

const char* POINT_TYPE_STRS[]= {
    [POINT_TYPE_CF]= "Control flow",
    [POINT_TYPE_DF]= "Data flow",
    [POINT_TYPE_SENTINEL]= "Sentinel"
};

const char* DF_POINT_REASON_STRS[]= {
    [DF_REASON_ARG_ASSIGN]= "Passed as arg and altered",
    [DF_REASON_VAR_ASSIGN]= "Variable assigned",
    [DF_REASON_MEMBER_ASSIGN]= "Member assigned",
    [DF_REASON_RETURN_ASSIGN]= "Return value assigned",
};

const char* CF_POINT_REASON_STRS[]= {
    [CF_REASON_FRAME_START]= "Function frame started",
    [CF_REASON_FRAME_END]= "Function frame ended",
    [CF_REASON_PRE_FUNC_CALL]= "Pre function call",
    [CF_REASON_POST_FUNC_CALL]= "Post function call",
    [CF_REASON_PRE_CONDITIONAL]= "Pre conditional",
    [CF_REASON_CONDITIONAL_RESOLUTION]= "Conditional resolved",
    [CF_REASON_CUSTOM]= "Custom"
};

#define MAKE_DF(addr_, reason_, die_offset_) MAKE_DF_LOC(addr_, reason_, die_offset_, (VLocation){0})

static const POINT SENTINEL= (POINT){.type= POINT_TYPE_SENTINEL};

typedef struct SubInfo {
    uintptr_t start;
    const uintptr_t ends[3];
    const POINT** points;
    bool placed_points;
} SubInfo;

static TracedFrame* root;
TracedFrame* current_frame;

#define MAKE_INFO(start_, points_, ...) (SubInfo) {\
    .start= start_,\
    .points= points_,\
    .ends= {__VA_ARGS__, 0}\
}

#define MAKE_POINTS(...) {__VA_ARGS__, &SENTINEL}

// pre-calls include args
// pre-conditional is on the line of the cmp

/*
 *      MAIN INFO
 */
static CF_POINT main_header= MAKE_CF(0x1131, CF_REASON_FRAME_START)
static CF_POINT main_before_call= MAKE_CF(0x1136, CF_REASON_PRE_FUNC_CALL)
static CF_POINT main_after_call= MAKE_CF(0x113b, CF_REASON_POST_FUNC_CALL)
static CF_POINT main_end= MAKE_CF(0x113b, CF_REASON_FRAME_END)

static DF_POINT main_ret= MAKE_DF(0x113b, DF_REASON_RETURN_ASSIGN, 0x32e)

const POINT* main_points[]= MAKE_POINTS(
    (POINT*)&main_header,
    (POINT*)&main_before_call,
    (POINT*)&main_after_call,
    (POINT*)&main_end,
    (POINT*)&main_ret
);

/*
 *      PARSE INFO
 */
static CF_POINT parse_header= MAKE_CF(0x1149, CF_REASON_FRAME_START)
static CF_POINT parse_end= MAKE_CF(0x1191, CF_REASON_FRAME_END)

static DF_POINT parse_tidx_assign= MAKE_DF(0x1154, DF_REASON_VAR_ASSIGN, 0x109)
static DF_POINT parse_maxtidx_assign= MAKE_DF(0x115f, DF_REASON_VAR_ASSIGN, 0x11e)

static CF_POINT parse_while_start= MAKE_CF(0x1179, CF_REASON_PRE_CONDITIONAL)
static CF_POINT parse_while_body= MAKE_CF(0x1161, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT parse_pre_call= MAKE_CF(0x1166, CF_REASON_PRE_FUNC_CALL)
static CF_POINT parse_post_call= MAKE_CF(0x116b, CF_REASON_POST_FUNC_CALL)

static DF_POINT parse_res_assign= MAKE_DF(0x116e, DF_REASON_VAR_ASSIGN, 0x31e)

static CF_POINT parse_if= MAKE_CF(0x116e, CF_REASON_PRE_CONDITIONAL)
static CF_POINT parse_if_body= MAKE_CF(0x1174, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT parse_else= MAKE_CF(0x1179, CF_REASON_CONDITIONAL_RESOLUTION)

static DF_POINT parse_return= MAKE_DF(0x1191, DF_REASON_RETURN_ASSIGN, 0x2ed)

const POINT* parse_points[]= MAKE_POINTS(
    (POINT*)&parse_header,
    (POINT*)&parse_end,
    (POINT*)&parse_tidx_assign,
    (POINT*)&parse_maxtidx_assign,
    (POINT*)&parse_while_start,
    (POINT*)&parse_while_body,
    (POINT*)&parse_pre_call,
    (POINT*)&parse_post_call,
    (POINT*)&parse_res_assign,
    (POINT*)&parse_if,
    (POINT*)&parse_if_body,
    (POINT*)&parse_else,
    (POINT*)&parse_return
);

/*
 *      PARSE_TOP_LEVEL INFO
 */
static CF_POINT top_header= MAKE_CF(0x1193, CF_REASON_FRAME_START)
static CF_POINT top_end= MAKE_CF(0x11ff, CF_REASON_FRAME_END)

static CF_POINT top_pre_current_call= MAKE_CF(0x11a4, CF_REASON_PRE_FUNC_CALL)
static CF_POINT top_post_call= MAKE_CF(0x11a9, CF_REASON_POST_FUNC_CALL)

static DF_POINT top_c_assign= MAKE_DF(0x11ad, DF_REASON_VAR_ASSIGN, 0x2db)

static CF_POINT top_if_condition= MAKE_CF(0x11ad, CF_REASON_PRE_CONDITIONAL)
static CF_POINT top_if_resolution= MAKE_CF(0x11b4, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT top_else= MAKE_CF(0x11bb, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT top_switch= MAKE_CF(0x11c1, CF_REASON_PRE_CONDITIONAL)
static CF_POINT top_res_x= MAKE_CF(0x11d6, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT top_res_y= MAKE_CF(0x11e2, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT top_res_z= MAKE_CF(0x11ee, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT top_res_default= MAKE_CF(0x11fa, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT top_pre_call_x= MAKE_CF(0x11db, CF_REASON_PRE_FUNC_CALL)
static CF_POINT top_pre_call_y= MAKE_CF(0x11e7, CF_REASON_PRE_FUNC_CALL)
static CF_POINT top_pre_call_z= MAKE_CF(0x11f3, CF_REASON_PRE_FUNC_CALL)

static CF_POINT top_post_call_x= MAKE_CF(0x11e0, CF_REASON_POST_FUNC_CALL)
static CF_POINT top_post_call_y= MAKE_CF(0x11ec, CF_REASON_POST_FUNC_CALL)
static CF_POINT top_post_call_z= MAKE_CF(0x11f8, CF_REASON_POST_FUNC_CALL)

static DF_POINT top_return_val_assign= MAKE_DF(0x11ff, DF_REASON_RETURN_ASSIGN, 0x2bb)

const POINT* top_points[]= MAKE_POINTS(
    (POINT*)&top_header,
    (POINT*)&top_end,
    (POINT*)&top_pre_current_call,
    (POINT*)&top_post_call,
    (POINT*)&top_c_assign,
    (POINT*)&top_if_condition,
    (POINT*)&top_if_resolution,
    (POINT*)&top_else,
    (POINT*)&top_switch,
    (POINT*)&top_res_x,
    (POINT*)&top_res_y,
    (POINT*)&top_res_z,
    (POINT*)&top_res_default,
    (POINT*)&top_pre_call_x,
    (POINT*)&top_pre_call_y,
    (POINT*)&top_pre_call_z,
    (POINT*)&top_post_call_x,
    (POINT*)&top_post_call_y,
    (POINT*)&top_post_call_z,
    (POINT*)&top_return_val_assign
);

/*
 *      PARSE_X_PRIME INFO
 */
static CF_POINT x_prime_header= MAKE_CF(0x120d, CF_REASON_FRAME_START)
static CF_POINT x_prime_pre_expect_y= MAKE_CF(0x1212, CF_REASON_PRE_FUNC_CALL)
static CF_POINT x_prime_post_expect_y= MAKE_CF(0x1217, CF_REASON_POST_FUNC_CALL)

static DF_POINT x_prime_y_assing= MAKE_DF(0x121b, DF_REASON_VAR_ASSIGN, 0x2ae)

static CF_POINT x_prime_pre_cond= MAKE_CF(0x121b, CF_REASON_PRE_CONDITIONAL)
static CF_POINT x_prime_cond_body= MAKE_CF(0x1222, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT x_prime_cond_else= MAKE_CF(0x1233, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT x_prime_pre_error= MAKE_CF(0x122c, CF_REASON_PRE_FUNC_CALL)
static CF_POINT x_prime_post_error= MAKE_CF(0x1231, CF_REASON_POST_FUNC_CALL)

static CF_POINT x_prime_pre_expect_two= MAKE_CF(0x1238, CF_REASON_PRE_FUNC_CALL)
static CF_POINT x_prime_post_expect_two= MAKE_CF(0x123d, CF_REASON_POST_FUNC_CALL)
static CF_POINT x_prime_pre_cond_two= MAKE_CF(0x123d, CF_REASON_PRE_CONDITIONAL)
static CF_POINT x_prime_cond_two_body= MAKE_CF(0x1242, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT x_prime_cond_two_else= MAKE_CF(0x1249, CF_REASON_CONDITIONAL_RESOLUTION)

static DF_POINT x_prime_ret_assign= MAKE_DF(0x124e, DF_REASON_RETURN_ASSIGN, 0x28c)

static CF_POINT x_prime_end= MAKE_CF(0x124e, CF_REASON_FRAME_END)

const POINT* x_prime_points[]= MAKE_POINTS(
    (POINT*)&x_prime_header,
    (POINT*)&x_prime_pre_expect_y,
    (POINT*)&x_prime_post_expect_y,
    (POINT*)&x_prime_y_assing,
    (POINT*)&x_prime_pre_cond,
    (POINT*)&x_prime_cond_body,
    (POINT*)&x_prime_cond_else,
    (POINT*)&x_prime_pre_error,
    (POINT*)&x_prime_post_error,
    (POINT*)&x_prime_pre_expect_two,
    (POINT*)&x_prime_post_expect_two,
    (POINT*)&x_prime_pre_cond_two,
    (POINT*)&x_prime_cond_two_body,
    (POINT*)&x_prime_cond_two_else,
    (POINT*)&x_prime_ret_assign,
    (POINT*)&x_prime_end
);

/*
 *      PARSE_X INFO
 */
static CF_POINT x_header= MAKE_CF(0x125c, CF_REASON_FRAME_START)
static CF_POINT x_end= MAKE_CF(0x1283, CF_REASON_FRAME_END)

static CF_POINT x_pre_consume= MAKE_CF(0x1261, CF_REASON_PRE_FUNC_CALL)
static CF_POINT x_post_consume= MAKE_CF(0x1266, CF_REASON_POST_FUNC_CALL)

static CF_POINT x_pre_x_prime= MAKE_CF(0x126b, CF_REASON_PRE_FUNC_CALL)
static CF_POINT x_post_x_prime= MAKE_CF(0x1270, CF_REASON_POST_FUNC_CALL)

static DF_POINT x_res_assign= MAKE_DF(0x1273, DF_REASON_VAR_ASSIGN, 0x27c)

static CF_POINT x_pre_cond= MAKE_CF(0x1273, CF_REASON_PRE_CONDITIONAL)
static CF_POINT x_cond_body= MAKE_CF(0x1279, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT x_cond_else= MAKE_CF(0x127e, CF_REASON_CONDITIONAL_RESOLUTION)

static DF_POINT x_ret_assign= MAKE_DF(0x1283, DF_REASON_RETURN_ASSIGN, 0x25c)

const POINT* x_points[]= MAKE_POINTS(
    (POINT*)&x_header,
    (POINT*)&x_end,
    (POINT*)&x_pre_consume,
    (POINT*)&x_post_consume,
    (POINT*)&x_pre_x_prime,
    (POINT*)&x_post_x_prime,
    (POINT*)&x_res_assign,
    (POINT*)&x_pre_cond,
    (POINT*)&x_cond_body,
    (POINT*)&x_cond_else,
    (POINT*)&x_ret_assign
);

/*
 *      PARSE_Y INFO
 */
static CF_POINT y_header= MAKE_CF(0x1285, CF_REASON_FRAME_START)
static CF_POINT y_end= MAKE_CF(0x12fc, CF_REASON_FRAME_END)

static CF_POINT y_pre_consume= MAKE_CF(0x1296, CF_REASON_PRE_FUNC_CALL);
static CF_POINT y_post_consume= MAKE_CF(0x129b, CF_REASON_POST_FUNC_CALL);

static DF_POINT y_y_assign= MAKE_DF(0x129f, DF_REASON_VAR_ASSIGN, 0x243)

static CF_POINT y_pre_expect= MAKE_CF(0x12a4, CF_REASON_PRE_FUNC_CALL)
static CF_POINT y_post_expect= MAKE_CF(0x12a9, CF_REASON_POST_FUNC_CALL)

static CF_POINT y_pre_if= MAKE_CF(0x12a9, CF_REASON_PRE_CONDITIONAL)
static CF_POINT y_if_body= MAKE_CF(0x12ae, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT y_if_else= MAKE_CF(0x12b5, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT y_pre_expect_two= MAKE_CF(0x12ba, CF_REASON_PRE_FUNC_CALL)
static CF_POINT y_post_expect_two= MAKE_CF(0x12bf, CF_REASON_POST_FUNC_CALL)

static DF_POINT y_x_assign= MAKE_DF(0x12c3, DF_REASON_VAR_ASSIGN, 0x24f)

static CF_POINT y_pre_if_two= MAKE_CF(0x12c3, CF_REASON_PRE_CONDITIONAL)
static CF_POINT y_if_body_two= MAKE_CF(0x12ca, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT y_if_else_two= MAKE_CF(0x12db, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT y_pre_error= MAKE_CF(0x12d4, CF_REASON_PRE_FUNC_CALL)
static CF_POINT y_post_error= MAKE_CF(0x12d9, CF_REASON_POST_FUNC_CALL)

static DF_POINT y_idx_assign= MAKE_DF_LOC(
    0x12e9,
    DF_REASON_MEMBER_ASSIGN,
    0x76,
    ((VLocation){.type= VLOCATION_REG_OFF, .data.reg_off= {.register_id= RAX_CODE, .offset= 0x8}})
)

static CF_POINT y_pre_expect_three= MAKE_CF(0x12ee, CF_REASON_PRE_FUNC_CALL)
static CF_POINT y_post_expect_three= MAKE_CF(0x12f3, CF_REASON_POST_FUNC_CALL)

static DF_POINT y_return_assign= MAKE_DF(0x12fc, DF_REASON_RETURN_ASSIGN, 0x223)

const POINT* y_points[]= MAKE_POINTS(
    (POINT*)&y_header,
    (POINT*)&y_end,
    (POINT*)&y_pre_consume,
    (POINT*)&y_post_consume,
    (POINT*)&y_y_assign,
    (POINT*)&y_pre_expect,
    (POINT*)&y_post_expect,
    (POINT*)&y_pre_if,
    (POINT*)&y_if_body,
    (POINT*)&y_if_else,
    (POINT*)&y_pre_expect_two,
    (POINT*)&y_post_expect_two,
    (POINT*)&y_x_assign,
    (POINT*)&y_pre_if_two,
    (POINT*)&y_if_body_two,
    (POINT*)&y_if_else_two,
    (POINT*)&y_pre_error,
    (POINT*)&y_post_error,
    (POINT*)&y_idx_assign,
    (POINT*)&y_pre_expect_three,
    (POINT*)&y_post_expect_three,
    (POINT*)&y_return_assign
);

/*
 *      PARSE_Z INFO
 */
static CF_POINT z_header= MAKE_CF(0x130a, CF_REASON_FRAME_START)
static CF_POINT z_end= MAKE_CF(0x1369, CF_REASON_FRAME_END)

static DF_POINT z_return_assign= MAKE_DF(0x1369, DF_REASON_RETURN_ASSIGN, 0x1ea)

static CF_POINT z_pre_consume= MAKE_CF(0x130f, CF_REASON_PRE_FUNC_CALL)
static CF_POINT z_post_consume= MAKE_CF(0x1314, CF_REASON_POST_FUNC_CALL)

static CF_POINT z_pre_expect= MAKE_CF(0x1319, CF_REASON_PRE_FUNC_CALL)
static CF_POINT z_post_expect= MAKE_CF(0x131e, CF_REASON_POST_FUNC_CALL)

static DF_POINT z_y_assign= MAKE_DF(0x1322, DF_REASON_VAR_ASSIGN, 0x20a)

static CF_POINT z_pre_cond= MAKE_CF(0x1322, CF_REASON_PRE_CONDITIONAL)
static CF_POINT z_cond_body= MAKE_CF(0x1329, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT z_cond_else= MAKE_CF(0x133a, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT z_pre_error= MAKE_CF(0x1333, CF_REASON_PRE_FUNC_CALL)
static CF_POINT z_post_error= MAKE_CF(0x1338, CF_REASON_POST_FUNC_CALL)

static CF_POINT z_pre_expect_two= MAKE_CF(0x133f, CF_REASON_PRE_FUNC_CALL)
static CF_POINT z_post_expect_two= MAKE_CF(0x1344, CF_REASON_POST_FUNC_CALL)

static DF_POINT z_z_assign= MAKE_DF(0x1348, DF_REASON_VAR_ASSIGN, 0x216)

static CF_POINT z_pre_cond_two= MAKE_CF(0x1348, CF_REASON_PRE_CONDITIONAL)
static CF_POINT z_cond_body_two= MAKE_CF(0x134f, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT z_cond_else_two= MAKE_CF(0x1356, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT z_pre_expect_three= MAKE_CF(0x135b, CF_REASON_PRE_FUNC_CALL);
static CF_POINT z_post_expect_three= MAKE_CF(0x1360, CF_REASON_POST_FUNC_CALL)

const POINT* z_points[]= MAKE_POINTS(
    (POINT*)&z_header,
    (POINT*)&z_end,
    (POINT*)&z_return_assign,
    (POINT*)&z_pre_consume,
    (POINT*)&z_post_consume,
    (POINT*)&z_pre_expect,
    (POINT*)&z_post_expect,
    (POINT*)&z_y_assign,
    (POINT*)&z_pre_cond,
    (POINT*)&z_cond_body,
    (POINT*)&z_cond_else,
    (POINT*)&z_pre_error,
    (POINT*)&z_post_error,
    (POINT*)&z_pre_expect_two,
    (POINT*)&z_post_expect_two,
    (POINT*)&z_z_assign,
    (POINT*)&z_pre_cond_two,
    (POINT*)&z_cond_body_two,
    (POINT*)&z_cond_else_two,
    (POINT*)&z_pre_expect_three,
    (POINT*)&z_post_expect_three
);

/*
 *      ERROR INFO
 */
static CF_POINT error_header= MAKE_CF(0x1377, CF_REASON_FRAME_START)
static CF_POINT error_end= MAKE_CF(0x137c, CF_REASON_FRAME_END)

static DF_POINT error_return_assign= MAKE_DF(0x137c, DF_REASON_RETURN_ASSIGN, 0x1ad)

const POINT* error_points[]= MAKE_POINTS(
    (POINT*)&error_header,
    (POINT*)&error_end,
    (POINT*)&error_return_assign
);

/*
 *      EXPECT INFO
 */
static CF_POINT expect_header= MAKE_CF(0x138d, CF_REASON_FRAME_START)
static CF_POINT expect_end= MAKE_CF(0x13cb, CF_REASON_FRAME_END)

static DF_POINT expect_return_assign= MAKE_DF(0x13cb, DF_REASON_RETURN_ASSIGN, 0x170)

static CF_POINT expect_pre_current= MAKE_CF(0x1392, CF_REASON_PRE_FUNC_CALL)
static CF_POINT expect_post_current= MAKE_CF(0x1397, CF_REASON_POST_FUNC_CALL)

static DF_POINT expect_c_assign= MAKE_DF(0x139b, DF_REASON_VAR_ASSIGN, 0x1a0)

static CF_POINT expect_pre_cond= MAKE_CF(0x139b, CF_REASON_PRE_CONDITIONAL)
static CF_POINT expect_cond_body= MAKE_CF(0x13a2, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT expect_cond_else= MAKE_CF(0x13a9, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT expect_pre_current_two= MAKE_CF(0x13ae, CF_REASON_PRE_FUNC_CALL)
static CF_POINT expect_post_current_two= MAKE_CF(0x13b3, CF_REASON_POST_FUNC_CALL)

static CF_POINT expect_pre_cond_two= MAKE_CF(0x13b5, CF_REASON_PRE_CONDITIONAL)
static CF_POINT expect_cond_body_two= MAKE_CF(0x13ba, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT expect_cond_else_two= MAKE_CF(0x13c1, CF_REASON_CONDITIONAL_RESOLUTION)

static CF_POINT expect_pre_consume= MAKE_CF(0x13c6, CF_REASON_PRE_FUNC_CALL)
static CF_POINT expect_post_consume= MAKE_CF(0x13cb, CF_REASON_POST_FUNC_CALL)

const POINT* expect_points[]= MAKE_POINTS(
    (POINT*)&expect_header,
    (POINT*)&expect_end,
    (POINT*)&expect_return_assign,
    (POINT*)&expect_pre_current,
    (POINT*)&expect_post_current,
    (POINT*)&expect_c_assign,
    (POINT*)&expect_pre_cond,
    (POINT*)&expect_cond_body,
    (POINT*)&expect_cond_else,
    (POINT*)&expect_pre_current_two,
    (POINT*)&expect_post_current_two,
    (POINT*)&expect_pre_cond_two,
    (POINT*)&expect_cond_body_two,
    (POINT*)&expect_cond_else_two,
    (POINT*)&expect_pre_consume,
    (POINT*)&expect_post_consume
);

/*
 *      CONSUME INFO
 */
static CF_POINT consume_header= MAKE_CF(0x13d5, CF_REASON_FRAME_START)
static CF_POINT consume_end= MAKE_CF(0x1412, CF_REASON_FRAME_END)

static DF_POINT consume_return_assign= MAKE_DF(0x1412, DF_REASON_RETURN_ASSIGN, 0x154)

static CF_POINT consume_pre_cond= MAKE_CF(0x13e3, CF_REASON_PRE_CONDITIONAL)
static CF_POINT consume_cond_body= MAKE_CF(0x13e8, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT consume_cond_else= MAKE_CF(0x13ef, CF_REASON_CONDITIONAL_RESOLUTION)

const POINT* consume_points[]= MAKE_POINTS(
    (POINT*)&consume_header,
    (POINT*)&consume_end,
    (POINT*)&consume_return_assign,
    (POINT*)&consume_pre_cond,
    (POINT*)&consume_cond_body,
    (POINT*)&consume_cond_else
);

/*
 *      CURRENT INFO
 */
static CF_POINT current_header= MAKE_CF(0x141c, CF_REASON_FRAME_START)
static CF_POINT current_end= MAKE_CF(0x144e, CF_REASON_FRAME_END)

static DF_POINT current_return_assign= MAKE_DF(0x144e, DF_REASON_RETURN_ASSIGN, 0x133)

static CF_POINT current_pre_cond= MAKE_CF(0x142a, CF_REASON_PRE_CONDITIONAL)
static CF_POINT current_cond_body= MAKE_CF(0x142f, CF_REASON_CONDITIONAL_RESOLUTION)
static CF_POINT current_cond_else= MAKE_CF(0x1436, CF_REASON_CONDITIONAL_RESOLUTION)

const POINT* current_points[]= MAKE_POINTS(
    (POINT*)&current_header,
    (POINT*)&current_end,
    (POINT*)&current_return_assign,
    (POINT*)&current_pre_cond,
    (POINT*)&current_cond_body,
    (POINT*)&current_cond_else
);

static SubInfo sub_infos[]= {
    MAKE_INFO(0x1129, main_points, 0x113b),
    MAKE_INFO(0x113d, parse_points, 0x1191),
    MAKE_INFO(0x1193, top_points, 0x11ff),
    MAKE_INFO(0x1201, x_prime_points, 0x124e),
    MAKE_INFO(0x1250, x_points, 0x1283),
    MAKE_INFO(0x1285, y_points, 0x12fc),
    MAKE_INFO(0x12fe, z_points, 0x1369),
    MAKE_INFO(0x136b, error_points, 0x137c),
    MAKE_INFO(0x137e, expect_points, 0x13cb),
    MAKE_INFO(0x13cd, consume_points, 0x1412),
    MAKE_INFO(0x1414, current_points, 0x144e)
};

StackFrame create_frame() {
    const uintptr_t pc= target.target_get_pc();
    const uintptr_t v_pc= target.target_addr_runtime_to_virtual(pc);

    bool succ;
    const uintptr_t cfa= target.target_get_cfa(&succ);

    VSub* vsub= target.target_get_vsub_at(v_pc);

    const AddrLineRes res= target.target_addr_to_line(v_pc);
    uint32_t line= -1;
    if (res.succ) line= res.line;

    const StackFrame frame= (StackFrame) {
        .pc= pc,
        .v_pc= v_pc,
        .cfa= cfa,
        .end_stack_pointer= 0,
        .sub= vsub,
        .vars= VVarInstance_arr_construct(0),
        .args= VVarInstance_arr_construct(0),
        .line= line
    };

    return frame;
}

TracedFrame* alloc_traced(TracedFrame* parent) {
    TracedFrame* frame= malloc(sizeof(TracedFrame));

    *frame= (TracedFrame) {
        .pos_in_parent= parent != NULL ? parent->points.pos : -1,
        .parent= parent,
        .frame= create_frame(),
        .points= PointInstance_vec_create(),
        .links= TracedFrame_vec_create(),
    };

    return frame;
}

void instance_all_params() {
    for (int i = 0; i < current_frame->frame.sub->params.pos; ++i) {
        VVar* param= VVar_vec_get_unsafe(&current_frame->frame.sub->params, i);

        VVarInstance inst= target.target_instance_var(param, current_frame->frame.cfa);
        VVarInstance_arr_add(&current_frame->frame.args, inst);
    }
}

CFPointInstance* cf_hit(CF_POINT* cf) {
    CFPointInstance* inst= malloc(sizeof(CFPointInstance));

    inst->base.type= POINT_TYPE_CF;
    inst->base.point= (POINT*)cf;

    if (cf->reason == CF_REASON_FRAME_START) instance_all_params();

    return inst;
}

DFPointInstance* df_hit(DF_POINT* df) {
    DFPointInstance* inst= malloc(sizeof(DFPointInstance));
    inst->base.type= POINT_TYPE_DF;
    inst->base.point= (POINT*)df;

    switch (df->reason) {
        case DF_REASON_VAR_ASSIGN: {
            VVar* var= target.target_get_virtual_from_ref(df->die_offset);
            inst->value= target.target_instance_var(var, current_frame->frame.cfa).value;
            break;
        }
        case DF_REASON_RETURN_ASSIGN: {
            inst->value.type= VALUE_GENERAL_VALUE;
            inst->value.data.general= target.target_get_reg(RAX_CODE).value.general;
            break;
        }
        case DF_REASON_MEMBER_ASSIGN: {
            VType* type= target.target_get_virtual_from_ref(df->die_offset);
            inst->value= target.target_instance_value(type, df->loc, current_frame->frame.cfa);
            break;
        }
        case DF_REASON_ARG_ASSIGN: {
            VVar* var= target.target_get_virtual_from_ref(df->die_offset);
            inst->value= target.target_instance_var(var, current_frame->frame.cfa).value;
            break;
        }
    }

    return inst;
}

void point_hit(void* v_point) {
    const POINT* point= v_point;

    PointInstance* inst;
    switch (point->type) {
        case POINT_TYPE_DF: {
            DF_POINT* df= v_point;
            inst= (PointInstance*)df_hit(df);
            break;
        }
        case POINT_TYPE_CF: {
            CF_POINT* cf= v_point;
            inst= (PointInstance*)cf_hit(cf);
            break;
        }
        default:
        case POINT_TYPE_SENTINEL: assert(false);
    }

    PointInstance_vec_add(&current_frame->points, inst);
}

int sub_info_search_cmp(const void* a, const void* b) {
    const uintptr_t s_addr= (uintptr_t)a;
    const SubInfo* sub= b;

    if (s_addr < sub->start) return -1;
    if (s_addr > sub->start) return 1;
    return 0;
}

void place_function_points(SubInfo* info) {
    if (info->placed_points) return;

    size_t idx= 0;
    while (info->points[idx]->type != POINT_TYPE_SENTINEL) {
        const POINT* point= info->points[idx];
        const uintptr_t r_addr= target.target_addr_virtual_to_runtime(point->addr);
        target.target_place_bp_with_cfa(
            r_addr,
            -1,
            BP_REASON_BREAK_SAVE,
            point_hit,
            (void*)point
        );
        idx++;
    }
    info->placed_points= true;
}


void function_hit(void* sub_start) {
    SubInfo* info= bsearch(
        sub_start,
        sub_infos,
        sizeof(sub_infos) / sizeof(sub_infos[0]),
        sizeof(sub_infos[0]),
        sub_info_search_cmp
    );

    if (!info) assert(false);

    TracedFrame* new_frame= alloc_traced(current_frame);

    if (current_frame == NULL) {
        root= new_frame;
    } else {
        TracedFrame_vec_add(&current_frame->links, new_frame);
    }

    current_frame= new_frame;
    place_function_points(info);
}

void function_end(void* ignored) {
    if (!current_frame) return;
    current_frame= current_frame->parent;
}

int sub_info_cmp(const void* a, const void* b) {
    const SubInfo* suba= a;
    const SubInfo* subb= b;

    if (suba->start < subb->start) return -1;
    if (suba->start > subb->start) return 1;
    return 0;
}

extern CONTROL_STATE state;
void break_save_end() {
    size_t idx= 0;
    g_idle_add(display_break_save_tree, root);
    state= STATE_NORMAL;
}

bool displayed= false;
void break_save_check_cond_callback(void* idngore) {
    const uint64_t rbp= target.target_get_reg(RBP_CODE).value.general;
    const uintptr_t addr= rbp - 0x4;
    int64_t data= target.target_get_general_data_runtime(addr, 4);

    if (data == -1 && !displayed) {
        displayed= true;
        break_save_end();
    }
}

int break_save() {
    qsort(
        sub_infos,
        sizeof(sub_infos) / sizeof(sub_infos[0]),
        sizeof(SubInfo),
        sub_info_cmp
    );

    bool succ;
    target.target_place_bp_defered(
        target.target_addr_virtual_to_runtime(0x116e),
        target.target_get_cfa(&succ),
        BP_REASON_BREAK_SAVE,
        break_save_check_cond_callback,
        NULL
    );

    const uintptr_t v_addr= target.target_addr_runtime_to_virtual(target.target_get_pc());
    const VSub* vsub= target.target_get_vsub_at(v_addr);
    function_hit((void*)vsub->vaddr_start);

    for (int i = 0; i < sizeof(sub_infos) / sizeof(sub_infos[0]); ++i) {
        const SubInfo* info= &sub_infos[i];
        uintptr_t r_addr= target.target_addr_virtual_to_runtime(info->start);

        target.target_place_bp_with_cfa(
            r_addr,
            -1,
            BP_REASON_BREAK_SAVE,
            function_hit,
            (void*)info->start
        );

        size_t idx= 0;
        while (info->ends[idx] != 0) {
            r_addr= target.target_addr_virtual_to_runtime(info->ends[idx]);

            target.target_place_bp_defered(
                r_addr,
                -1,
                BP_REASON_BREAK_SAVE,
                function_end,
                NULL
            );
            idx++;
        }
    }

    return SUCCESS;
}
