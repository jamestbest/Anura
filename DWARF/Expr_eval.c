//
// Created by jamestbest on 4/5/26.
//

#include "Expr_eval.h"

typedef enum DW_OP {
    DW_OP_addr= 0x03,
    DW_OP_deref= 0x06,
    DW_OP_const1u= 0x08,
    DW_OP_const1s= 0x09,
    DW_OP_const2u= 0x0a,
    DW_OP_const2s= 0x0b,
    DW_OP_const4u= 0x0c,
    DW_OP_const4s= 0x0d,
    DW_OP_const8u= 0x0e,
    DW_OP_const8s= 0x0f,
    DW_OP_constu= 0x10,
    DW_OP_consts= 0x11,
    DW_OP_dup= 0x12,
    DW_OP_drop= 0x13,
    DW_OP_over= 0x14,
    DW_OP_pick= 0x15,
    DW_OP_swap= 0x16,
    DW_OP_rot= 0x17,
    DW_OP_xderef= 0x18,
    DW_OP_abs= 0x19,
    DW_OP_and= 0x1a,
    DW_OP_div= 0x1b,
    DW_OP_minus= 0x1c,
    DW_OP_mod= 0x1d,
    DW_OP_mul= 0x1e,
    DW_OP_neg= 0x1f,
    DW_OP_not= 0x20,
    DW_OP_or= 0x21,
    DW_OP_plus= 0x22,
    DW_OP_plus_uconst= 0x23,
    DW_OP_shl= 0x24,
    DW_OP_shr= 0x25,
    DW_OP_shra= 0x26,
    DW_OP_xor= 0x27,
    DW_OP_bra= 0x28,
    DW_OP_eq= 0x29,
    DW_OP_ge= 0x2a,
    DW_OP_gt= 0x2b,
    DW_OP_le= 0x2c,
    DW_OP_lt= 0x2d,
    DW_OP_ne= 0x2e,
    DW_OP_skip= 0x2f,
    DW_OP_lit_min= 0x30,
    DW_OP_lit_max= 0x4f,
    DW_OP_reg_min= 0x50,
    DW_OP_reg_max= 0x6f,
    DW_OP_breg_min= 0x70,
    DW_OP_breg_max= 0x8f,
    DW_OP_regx= 0x90,
    DW_OP_fbreg= 0x91,
    DW_OP_bregx= 0x92,
    DW_OP_piece= 0x93,
    DW_OP_deref_size= 0x94,
    DW_OP_xderef_size= 0x95,
    DW_OP_nop= 0x96,
    DW_OP_push_object_address= 0x97,
    DW_OP_call2= 0x98,
    DW_OP_call4= 0x99,
    DW_OP_call_ref= 0x9a,
    DW_OP_form_tls_address= 0x9b,
    DW_OP_call_frame_cfa= 0x9c,
    DW_OP_bit_piece= 0x9d,
    DW_OP_implicit_value= 0x9e,
    DW_OP_stack_value= 0x9f,
    DW_OP_implicit_pointer= 0xa0,
    DW_OP_addrx= 0xa1,
    DW_OP_constx= 0xa2,
    DW_OP_entry_value= 0xa3,
    DW_OP_const_type= 0xa4,
    DW_OP_regval_type= 0xa5,
    DW_OP_deref_type= 0xa6,
    DW_OP_xderef_type= 0xa7,
    DW_OP_convert= 0xa8,
    DW_OP_reinterpret= 0xa9,
    DW_OP_lo_user= 0xe0,
    DW_OP_hi_user= 0xff,
} DW_OP;

typedef uintptr_t generic;
ARRAY_PROTO(generic, generic);
ARRAY_ADD(generic, generic);

VLocation execute_op(uint8_t** base, genericArray* stack, bool* complete, uintptr_t fbreg) {
    DW_OP opcode= raa_uint(base, 1);

    if (opcode >= DW_OP_lit_min && opcode <= DW_OP_lit_max) {
        generic_arr_add(stack, opcode - DW_OP_lit_min);
        goto not_complete;
    }

    if (opcode >= DW_OP_reg_min && opcode <= DW_OP_reg_max) {
        VLocation loc;
        loc.type= VLOCATION_REGISTER;
        loc.data.register_id= opcode - DW_OP_reg_min;
        *complete= true;
        return loc;
    }

    if (opcode >= DW_OP_breg_min && opcode <= DW_OP_breg_max) {
        LEB128 offset= raa_leb128(base);
        uint8_t reg= opcode - DW_OP_breg_min;
        uintptr_t reg_value= target.target_get_reg(reg).value.general;
        uintptr_t addr= reg_value + offset.v;
        generic_arr_add(stack, addr);
        goto not_complete;
    }

    switch (opcode) {
        case DW_OP_addr: {
            uintptr_t addr= raa_uint(base, 8);
            generic_arr_add(stack, addr);
            goto not_complete;
        }
        case DW_OP_deref:
            break;
        case DW_OP_const1u:
            break;
        case DW_OP_const1s:
            break;
        case DW_OP_const2u:
            break;
        case DW_OP_const2s:
            break;
        case DW_OP_const4u:
            break;
        case DW_OP_const4s:
            break;
        case DW_OP_const8u:
            break;
        case DW_OP_const8s:
            break;
        case DW_OP_constu:
            break;
        case DW_OP_consts:
            break;
        case DW_OP_dup:
            break;
        case DW_OP_drop:
            break;
        case DW_OP_over:
            break;
        case DW_OP_pick:
            break;
        case DW_OP_swap:
            break;
        case DW_OP_rot:
            break;
        case DW_OP_xderef:
            break;
        case DW_OP_abs:
            break;
        case DW_OP_and:
            break;
        case DW_OP_div:
            break;
        case DW_OP_minus:
            break;
        case DW_OP_mod:
            break;
        case DW_OP_mul:
            break;
        case DW_OP_neg:
            break;
        case DW_OP_not:
            break;
        case DW_OP_or:
            break;
        case DW_OP_plus:
            break;
        case DW_OP_plus_uconst:
            break;
        case DW_OP_shl:
            break;
        case DW_OP_shr:
            break;
        case DW_OP_shra:
            break;
        case DW_OP_xor:
            break;
        case DW_OP_bra:
            break;
        case DW_OP_eq:
            break;
        case DW_OP_ge:
            break;
        case DW_OP_gt:
            break;
        case DW_OP_le:
            break;
        case DW_OP_lt:
            break;
        case DW_OP_ne:
            break;
        case DW_OP_skip:
            break;
        case DW_OP_regx:
            break;
        case DW_OP_fbreg: {
            const LEB128 offset= raa_leb128(base);
            const uintptr_t addr= fbreg + offset.v;
            generic_arr_add(stack, addr);
            goto not_complete;
        }
        case DW_OP_bregx:
            break;
        case DW_OP_piece:
            break;
        case DW_OP_deref_size:
            break;
        case DW_OP_xderef_size:
            break;
        case DW_OP_nop:
            break;
        case DW_OP_push_object_address:
            break;
        case DW_OP_call2:
            break;
        case DW_OP_call4:
            break;
        case DW_OP_call_ref:
            break;
        case DW_OP_form_tls_address:
            break;
        case DW_OP_call_frame_cfa:
            bool succ;
            const uintptr_t addr= target.target_get_cfa(&succ);
            generic_arr_add(stack, addr);
            goto not_complete;
        case DW_OP_bit_piece:
            break;
        case DW_OP_implicit_value:
            break;
        case DW_OP_stack_value:
            break;
        case DW_OP_implicit_pointer:
            break;
        case DW_OP_addrx:
            break;
        case DW_OP_constx:
            break;
        case DW_OP_entry_value:
            break;
        case DW_OP_const_type:
            break;
        case DW_OP_regval_type:
            break;
        case DW_OP_deref_type:
            break;
        case DW_OP_xderef_type:
            break;
        case DW_OP_convert:
            break;
        case DW_OP_reinterpret:
            break;
        case DW_OP_lo_user:
            break;
        case DW_OP_hi_user:
            break;
    }

    assert(false);

not_complete:
    *complete= false;
    return (VLocation){0};
}

VLocation eval_dw_expr_to_location(DW_EXPR* expr, uintptr_t fbreg) {
    genericArray stack= generic_arr_create();

    uint8_t* base= expr->data;
    while (base < expr->data + expr->length) {
        bool complete;
        const VLocation loc= execute_op(&base, &stack, &complete, fbreg);

        if (complete) return loc;
    }

    return (VLocation) {
        .type= VLOCATION_ADDR,
        .data.vaddr= generic_arr_pop(&stack)
    };
}
