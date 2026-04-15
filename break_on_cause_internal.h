//
// Created by jamestbest on 4/4/26.
//

#ifndef ANURA_BREAK_ON_CAUSE_INTERNAL_H
#define ANURA_BREAK_ON_CAUSE_INTERNAL_H

#include "break_on_cause.h"

#define ID(num) num
#define SUB_INFO(name, start, ...) const SubInfo name##_info= (SubInfo) {\
    .func_start= start,\
    .func_ends= {__VA_ARGS__, FUNC_ENDS_END}\
};

#define NO_PASS false
#define PASS true

#define NOT_DEAD false
#define IS_DEAD true

#define NOT_LEFT false
#define IS_LEFT true

#define SUCCESS 0
#define FAIL (-1)

DEP_BASE SENTINEL= (DEP_BASE) {.type= DEPTYPE_SENTINEL, .dead=false};

DEP_LINEAR global_tidx= MAKE_DATA(
    ID(100),
    LOC_REG_OFF(RIP_CODE, 0x2ca5 + 7, 8), // + 7 for the length of the instr
    0x1423
)

DEP_LINEAR global_max_tidx= MAKE_DATA(
    ID(101),
    LOC_REG_OFF(RIP_CODE, 0x2ca6 + 7, 8),
    0x142a
);

DEP_EXPR current_expr= MAKE_EXPR(
    ID(4),
    NOT_DEAD,
    NO_PASS,
    OP_GT,
    global_tidx,
    global_max_tidx,
    TYPE_NONE,
    0x141c,
    17,
    LOC_COMPARE(COMPARE_GT)
);

DEP_LINEAR current_zero= MAKE_CONST(
    ID(3),
    0,
    0x142f
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
    0x144e
);

DEP_SPLIT current_split= MAKE_SPLIT(
    ID(1),
    NOT_DEAD,
    (DEP_BASE*)&current_cond,
    (DEP_BASE*)&current_return
);

DEP_HEADER current_header= MAKE_HEADER(
    ID(0),
    0x1414,
    current_split,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR consume_return= MAKE_DATA(
    ID(11),
    LOC_RAX,
    0x1412
);

DEP_LINEAR consume_zero= MAKE_CONST(
    ID(10),
    0,
    0x13e8
);

DEP_EXPR consume_expr= MAKE_EXPR(
    ID(9),
    NOT_DEAD,
    NO_PASS,
    OP_GT,
    global_tidx,
    global_max_tidx,
    TYPE_NONE,
    0x13d5,
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
    (DEP_BASE*)&consume_cond,
    (DEP_BASE*)&consume_return
);

DEP_HEADER consume_header= MAKE_HEADER(
    ID(6),
    0x13cd,
    consume_split,
    NOT_DEAD,
    NO_PASS
);




DEP_LINEAR error_return= MAKE_CONST(
    ID(13),
    FAIL,
    0x1377
);

DEP_HEADER error_header= MAKE_HEADER(
    ID(12),
    0x1377,
    error_return,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR expect_consume= MAKE_SINGLE(
    ID(16),
    0x13c6,
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
    0x139b
);

DEP_LINEAR expect_local_c= MAKE_SINGLE(
    ID(20),
    0x1392,
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
    0x1397,
    9,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR expect_first_zero= MAKE_CONST(
    ID(21),
    0,
    0x13a2
);

DEP_COND expect_null_check= MAKE_COND(
    ID(17),
    NOT_DEAD,
    IS_LEFT,
    expect_expr_not_c,
    expect_first_zero
);

DEP_LINEAR expect_second_zero= MAKE_CONST(
    ID(23),
    0,
    0x13ba
);

DEP_LINEAR expect_expr_param_type= MAKE_DATA(
    ID(24),
    LOC_REG_OFF(RBP_CODE, -0x14, 4),
    0x13b5
);

DEP_LINEAR expect_current_type= MAKE_SINGLE(
    ID(26),
    0x13ae,
    7,
    LOC_EAX,
    IS_DEAD,
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
    0x13ae,
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
    0x138d,
    expect_split,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR parse_z_expect_z_call_2= MAKE_SINGLE(
    ID(30),
    0x135b,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_Z)"
);

DEP_LINEAR expect_z_const_zero= MAKE_CONST(
    ID(31),
    0,
    0x1360
);

DEP_EXPR parse_z_no_z_check= MAKE_EXPR(
    ID(29),
    NOT_DEAD,
    NO_PASS,
    OP_EQ,
    parse_z_expect_z_call_2,
    expect_z_const_zero,
    TYPE_LEFT,
    0x135b,
    8,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR parse_z_const_fail= MAKE_CONST(
    ID(33),
    FAIL,
    0x134f
);

DEP_LINEAR parse_z_const_zero_in_expr_not_z= MAKE_CONST(
    ID(35),
    0,
    0x1348
);

DEP_LINEAR parse_z_local_z= MAKE_SINGLE(
    ID(36),
    0x133f,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_Z)"
);

DEP_EXPR parse_z_expr_not_z= MAKE_EXPR(
    ID(34),
    NOT_DEAD,
    PASS,
    OP_EQ,
    parse_z_local_z,
    parse_z_const_zero_in_expr_not_z,
    TYPE_LEFT,
    0x1348,
    5,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_COND Parse_z_z_check= MAKE_COND(
    ID(32),
    NOT_DEAD,
    IS_LEFT,
    parse_z_expr_not_z,
    parse_z_const_fail
);

DEP_LINEAR parse_z_const_zero_in_expr_not_y= MAKE_CONST(
    ID(39),
    0,
    0x1322
);

DEP_LINEAR parse_z_local_y= MAKE_SINGLE(
    ID(40),
    0x1319,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_Y)"
);

DEP_EXPR parse_z_expr_not_y= MAKE_EXPR(
    ID(38),
    IS_DEAD,
    PASS,
    OP_EQ,
    parse_z_local_y,
    parse_z_const_zero_in_expr_not_y,
    TYPE_LEFT,
    0x1322,
    5,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR parse_z_error_call= MAKE_SINGLE(
    ID(41),
    0x1333,
    5,
    LOC_RAX,
    false,
    NO_PASS,
    error_header,
    "error(\"What?!\")"
);

DEP_COND parse_z_y_check= MAKE_COND(
    ID(37),
    NOT_DEAD,
    NOT_LEFT,
    parse_z_expr_not_y,
    parse_z_error_call
);

DEP_SPLIT parse_z_split= MAKE_SPLIT(
    ID(28),
    NOT_DEAD,
    (DEP_BASE*)&parse_z_y_check,
    (DEP_BASE*)&Parse_z_z_check,
    (DEP_BASE*)&parse_z_no_z_check
);

DEP_HEADER parse_z_header= MAKE_HEADER(
    ID(27),
    0x12fe,
    parse_z_split,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR parse_y_expr_const_zero= MAKE_CONST(
    ID(46),
    0,
    0x12a9
);

DEP_LINEAR parse_y_expr_expect_z= MAKE_SINGLE(
    ID(47),
    0x12a4,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_Z)"
);

DEP_EXPR parse_y_cond_expr_not_z= MAKE_EXPR(
    ID(45),
    NOT_DEAD,
    NO_PASS,
    OP_EQ,
    parse_y_expr_expect_z,
    parse_y_expr_const_zero,
    TYPE_LEFT,
    0x12a4,
    8,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR parse_y_cond_link_const_fail= MAKE_CONST(
    ID(48),
    FAIL,
    0x12ae
);

DEP_COND parse_y_cond_on_not_z= MAKE_COND(
    ID(44),
    NOT_DEAD,
    IS_LEFT,
    parse_y_cond_expr_not_z,
    parse_y_cond_link_const_fail
);

DEP_LINEAR parse_y_expr_const_zero_in_expr_not_x= MAKE_CONST(
    ID(51),
    0,
    0x12c3
);

DEP_LINEAR parse_y_local_x= MAKE_SINGLE(
    ID(52),
    0x12ba,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_X)"
);

DEP_EXPR parse_y_cond_expr_not_x= MAKE_EXPR(
    ID(50),
    IS_DEAD,
    PASS,
    OP_EQ,
    parse_y_local_x,
    parse_y_expr_const_zero_in_expr_not_x,
    TYPE_LEFT,
    0x12c3,
    5,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR parse_y_cond_link_error= MAKE_SINGLE(
    ID(53),
    0x12d4,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    error_header,
    "error(\"\")"
);

DEP_COND parse_y_cond_on_not_x= MAKE_COND(
    ID(49),
    NOT_DEAD,
    NOT_LEFT,
    parse_y_cond_expr_not_x,
    parse_y_cond_link_error
);

DEP_LINEAR parse_y_expr_const_zero_in_last_expr= MAKE_CONST(
    ID(55),
    0,
    0x12f3
);

DEP_LINEAR parse_y_expr_expect_x= MAKE_SINGLE(
    ID(56),
    0x12ee,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_X)"
);

DEP_EXPR parse_y_expr_on_not_x= MAKE_EXPR(
    ID(54),
    NOT_DEAD,
    NO_PASS,
    OP_EQ,
    parse_y_expr_expect_x,
    parse_y_expr_const_zero_in_last_expr,
    TYPE_LEFT,
    0x12ee,
    8,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_SPLIT parse_y_split= MAKE_SPLIT(
    ID(43),
    NOT_DEAD,
    (DEP_BASE*)&parse_y_cond_on_not_z,
    (DEP_BASE*)&parse_y_cond_on_not_x,
    (DEP_BASE*)&parse_y_expr_on_not_x
);

DEP_HEADER parse_y_header= MAKE_HEADER(
    ID(42),
    0x1285,
    parse_y_split,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR parse_prime_ret_success= MAKE_CONST(
    ID(67),
    SUCCESS,
    0x1249
);

DEP_LINEAR parse_prime_const_fail= MAKE_CONST(
    ID(69),
    FAIL,
    0x1242
);

DEP_LINEAR parse_prime_expr_const_zero= MAKE_CONST(
    ID(71),
    0,
    0x123d
);

DEP_LINEAR parse_prime_expr_expect_type_y= MAKE_SINGLE(
    ID(72),
    0x1238,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_Y)"
);

DEP_EXPR parse_prime_expr_not_type_y= MAKE_EXPR(
    ID(70),
    NOT_DEAD,
    NO_PASS,
    OP_EQ,
    parse_prime_expr_expect_type_y,
    parse_prime_expr_const_zero,
    TYPE_LEFT,
    0x1238,
    8,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_COND parse_prime_cond_on_not_type_y= MAKE_COND(
    ID(68),
    NOT_DEAD,
    IS_LEFT,
    parse_prime_expr_not_type_y,
    parse_prime_const_fail
);

DEP_LINEAR parse_prime_const_zero= MAKE_CONST(
    ID(75),
    0,
    0x121b
);

DEP_LINEAR parse_prime_local_y= MAKE_SINGLE(
    ID(76),
    0x1212,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    expect_header,
    "expect(TYPE_Y)"
);

DEP_EXPR parse_prime_expr_not_y= MAKE_EXPR(
    ID(74),
    IS_DEAD,
    PASS,
    OP_EQ,
    parse_prime_local_y,
    parse_prime_const_zero,
    TYPE_LEFT,
    0x121b,
    5,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR parse_prime_ret_error= MAKE_SINGLE(
    ID(77),
    0x122c,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    error_header,
    "error(\"Where is my Y!\")"
);

DEP_COND parse_prime_cond_on_y= MAKE_COND(
    ID(73),
    NOT_DEAD,
    NOT_LEFT,
    parse_prime_expr_not_y,
    parse_prime_ret_error
);

DEP_SPLIT parse_prime_split= MAKE_SPLIT(
    ID(66),
    NOT_DEAD,
    (DEP_BASE*)&parse_prime_ret_success,
    (DEP_BASE*)&parse_prime_cond_on_not_type_y,
    (DEP_BASE*)&parse_prime_cond_on_y
);

DEP_HEADER parse_x_prime_header= MAKE_HEADER(
    ID(65),
    0x1201,
    parse_prime_split,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR parse_x_ret_success= MAKE_CONST(
    ID(59),
    SUCCESS,
    0x127e
);

DEP_LINEAR parse_x_const_success= MAKE_CONST(
    ID(62),
    SUCCESS,
    0x1273
);

DEP_LINEAR parse_x_local_prime_res= MAKE_SINGLE(
    ID(64),
    0x126b,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    parse_x_prime_header,
    "parse_x_prime()"
);

DEP_COLLECT parse_x_collect= MAKE_COLLECT(
    ID(63),
    CONNECTIONS(2),
    NOT_DEAD,
    NO_PASS,
    parse_x_local_prime_res
);

DEP_EXPR parse_x_cond_not_success= MAKE_EXPR(
    ID(61),
    NOT_DEAD,
    PASS,
    OP_NE,
    parse_x_collect,
    parse_x_const_success,
    TYPE_LEFT,
    0x1273,
    4,
    LOC_COMPARE(COMPARE_NEQ)
);

DEP_COND parse_x_cond= MAKE_COND(
    ID(60),
    NOT_DEAD,
    NOT_LEFT,
    parse_x_cond_not_success,
    parse_x_collect
);

DEP_SPLIT parse_x_split= MAKE_SPLIT(
    ID(58),
    NOT_DEAD,
    (DEP_BASE*)&parse_x_cond,
    (DEP_BASE*)&parse_x_ret_success
);

DEP_HEADER parse_x_header= MAKE_HEADER(
    ID(57),
    0x1250,
    parse_x_split,
    NOT_DEAD,
    NO_PASS
);

DEP_LINEAR parse_top_const_zero= MAKE_CONST(
    ID(82),
    0,
    0x11ad
);

DEP_LINEAR parse_top_local_c= MAKE_SINGLE(
    ID(89),
    0x11a4,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    current_header,
    "current()"
);

DEP_EXPR parse_top_expr_not_c= MAKE_EXPR(
    ID(81),
    NOT_DEAD,
    PASS,
    OP_EQ,
    parse_top_local_c,
    parse_top_const_zero,
    TYPE_LEFT,
    0x11ad,
    5,
    LOC_COMPARE(COMPARE_EQ)
);

DEP_LINEAR parse_top_expr_const_fail= MAKE_CONST(
    ID(84),
    FAIL,
    0x11b4
);

DEP_COND parse_top_cond_not_c= MAKE_COND(
    ID(80),
    NOT_DEAD,
    IS_LEFT,
    parse_top_expr_not_c,
    parse_top_expr_const_fail
);

DEP_LINEAR parse_top_parse_x= MAKE_SINGLE(
    ID(85),
    0x11db,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    parse_x_header,
    "parse_x()"
);

DEP_LINEAR parse_top_parse_y= MAKE_SINGLE(
    ID(86),
    0x11e7,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    parse_y_header,
    "parse_y()"
);

DEP_LINEAR parse_top_parse_z= MAKE_SINGLE(
    ID(87),
    0x11f3,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    parse_z_header,
    "parse_z()"
);

DEP_LINEAR parse_top_ret_fail= MAKE_CONST(
    ID(88),
    FAIL,
    0x11fa
);

DEP_SPLIT parse_top_split= MAKE_SPLIT(
    ID(79),
    NOT_DEAD,
    (DEP_BASE*)&parse_top_cond_not_c,
    (DEP_BASE*)&parse_top_parse_x,
    (DEP_BASE*)&parse_top_parse_y,
    (DEP_BASE*)&parse_top_parse_z,
    (DEP_BASE*)&parse_top_ret_fail
);

DEP_HEADER top_header= MAKE_HEADER(
    ID(78),
    0x1193,
    parse_top_split,
    NOT_DEAD,
    NO_PASS
);


DEP_LINEAR parse_local_res= MAKE_SINGLE(
    ID(90),
    0x1166,
    5,
    LOC_RAX,
    NOT_DEAD,
    NO_PASS,
    top_header,
    "parse_top_level()"
);



SUB_INFO(error, 0x1377, 0x137c)
SUB_INFO(expect, 0x138d, 0x13cb)
SUB_INFO(consume, 0x13cd, 0x1412)
SUB_INFO(current, 0x1414, 0x144e)
SUB_INFO(parse_z, 0x12fe, 0x1369)
SUB_INFO(parse_y, 0x1285, 0x12fc)
SUB_INFO(parse_x, 0x1250, 0x1283)
SUB_INFO(parse_x_prime, 0x1201, 0x124e)
SUB_INFO(parse_top_level, 0x1193, 0x11ff)
SUB_INFO(parse_func, 0x113d, 0x1191)
SUB_INFO(main_example, 0x1129, 0x113b)

const SubInfo* example_sub_infos[]= {
    &error_info,
    &expect_info,
    &consume_info,
    &current_info,
    &parse_z_info,
    &parse_y_info,
    &parse_x_info,
    &parse_x_prime_info,
    &parse_top_level_info,
    &parse_func_info,
    &main_example_info,
    NULL
};


#endif //ANURA_BREAK_ON_CAUSE_INTERNAL_H