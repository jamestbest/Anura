//
// Created by James Coward on 1/27/26.
//

#include <stdint.h>

#include "Vector.h"
#include "MtDoom/output/default.h"
#include "shared/Buffer.h"

Buffer output;

// todo: this should also include the settings for parsing from the file
ParseRet parse(uint8_t* raw_stream) {
    output= buffer_create(BUFF_MIN);
    stream= (ByteStream) {.raw_stream= raw_stream, .pointer= 0};

    ParseRet ret= parse_structure();

    ret.bits_read= stream.pointer;
    return ret;
}

//STRUCTURE lprefix* prefix? op
ParseRet parse_structure() {
    while (parse_lprefix()) {}

    parse_prefix();

    parse_op();
}

//FLAG mode= 64bit | 32bit | 16bit default 64bit
typedef enum FLAG_MODE {
    FLAG_MODE_64bit,
    // ...
} FLAG_MODE;

FLAG_MODE flag_mode= FLAG_MODE_64bit;

//DATA ow 1 bit
typedef struct DATA_OW {
    uint8_t _value: 1;
} DATA_OW;
DATA_OW data_ow;

uint64_t FLAG_OW= 0;

/*
    DATA REX 1 BYTE= {
        0100 .w .r .x .b
    }
*/
typedef struct DATA_REX {
    uint8_t w: 1;
    uint8_t r: 1;
    uint8_t x: 1;
    uint8_t b: 1;
} DATA_REX;

DATA_REX data_rex;

ParseRet parse_data_rex() {
    if (peek_data(4) != 0b0100) {
        return (ParseRet){.success= false};
    }

    data_rex.w= read_bit(&stream);
    data_rex.r= read_bit(&stream);
    data_rex.x= read_bit(&stream);
    data_rex.b= read_bit(&stream);

    return 0;
}

/*
    DATA VEX x BYTES= {
        1100 0100 .R .X .B .m(5) .W .v(4) .L .pp(2)
        1100 0101 .R .v(4) .L .pp(2)
    }
*/
typedef struct DATA_VEX_0 {
    uint8_t R: 1;
    uint8_t X: 1;
    uint8_t B: 1;
    uint8_t m: 5;
    uint8_t W: 1;
    uint8_t v: 4;
    uint8_t L: 1;
    uint8_t pp: 2;
} DATA_VEX_0;

typedef struct DATA_VEX_1 {
    uint8_t R: 1;
    uint8_t v: 4;
    uint8_t L: 1;
    uint8_t pp: 2;
} DATA_VEX_1;

typedef struct DATA_VEX {
    DATA_VEX_0 VEX_0;
    DATA_VEX_1 VEX_1;
    uint64_t oval;
} DATA_VEX;

DATA_VEX data_vex;

void init_data() {
    data_rex= (DATA_REX){0};
    data_vex= (DATA_VEX){0};
}

// ALIAS lprefix 1 BYTE= {
//     lp1
//     lp2
//     lp3
//     lp4
// }
ParseRet parse_lprefix() {
    ParseRet ret;

    if (ret= parse_lp1(), ret.success) return ret;
    if (ret= parse_lp2(), ret.success) return ret;
    if (ret= parse_lp3(), ret.success) return ret;
    if (ret= parse_lp4(), ret.success) return ret;

    return (ParseRet){.success= false, .error_string= "Unable to match lprefix to lp1, lp2, lp3, or lp4"};
}

typedef struct AVAL {
    Vector choices;
    char* chosen_val;
    uint8_t chosen_idx;
} AVAL;

AVAL AVAL_lp1;

// ALIAS lp1 1 BYTE= {
//     0xF0= LOCK
//     0xF2= REPNE,REPNZ,BND  // BND has some restrictions to do with what the instr is
//     0xF3= REP,REPE,REPZ
// }
ParseRet parse_lp1() {
    if (expect_bits(8, 0xF0)) AVAL_lp1= create_aval(1, "LOCK"); return (ParseRet){.success= true};
    if (expect_bits(8, 0xF2)) AVAL_lp1= create_aval(3, "REPNE", "REPNZ", "BND"); return (ParseRet){.success= true};
    if (expect_bits(8, 0xF3)) AVAL_lp1= create_aval(3, "REP", "REPE", "REPZ"); return (ParseRet){.success= true};

    return (ParseRet) {.success= false};
}

// RULE RIGHT ON reg, regT {
//     CHOOSE 0 if opmode == 16bit
//     CHOOSE 1 if opmode == 8bit and REX.w != 1
//     CHOOSE 2 if opmode == 8bit
//     CHOOSE 3 if opmode == 32bit
//     CHOOSE 4 if opmode == 64bit
// }
ParseRet use_alias(const char* alias_name) {
    if (strcmp(alias_name, "reg") == 0) {
        use_alias_reg();
    }
}

AVAL AVAL_reg;

ParseRet use_alias_reg() {
    for (int i = 0; i < RULE_00.statements.pos; ++i) {
        ChooseStatement* stmt= RULE_00.statements.get(i);
        Expr* expr= stmt->expr;
        uint16_t choice= stmt->choice;

        if (evalulate_bool_expr(expr)) {
            AVAL_reg.chosen_idx= choice;
            AVAL_reg.chosen_val= vector_get_unsafe(&AVAL_lp1.choices, choice);
        }
    }
}

// DATA sibs 2 BITS= {.val(2)}
typedef struct DATA_SIBS {
    uint8_t val: 2;
} DATA_SIBS;

//ALIAS sibi= reg
AVAL AVAL_SIBI;

// ALIAS sibsi 5 BITS= {
//     sibs 100 = ""
//     sibs sibi= "[{sibi} * {2 ^ sibs.val}]"
// }


/*
    ALIAS ModRM= {
        11 reg 100 SIB= SIB
        00 reg 101 disp32= {
            when mode == 64bit then "rip + {disp32}"
            when default then disp32
        }
        00 reg RM= "[{reg}]"
        01 reg RM disp8 = "[{reg} + {disp8}]"
        10 reg RM disp32= "[{reg} + {disp32}]"
        11 reg RM= reg
    }
 */

ParseRet parse_ModRM() {
    if (EXPECT_BITS(0b11) && parse_reg() && EXPECT_BITS(0b100) && parse_SIB()) {
        AVAL_MODRM= AVAL_SIB;
        return;
    }

    if (EXPECT_BITS(0b00) && parse_reg && EXPECT_BITS(0b101) && parse_disp32()) {
        if (flag_mode == FLAG_MODE_64bit) {
            AVAL_MODRM= eval_string("rip + {disp32}");
            return;
        }
        AVAL_MODRM= data_to_aval(DATA_DISP32);
        return;
    }
}

typedef struct DATA_DISP32 {
    uint8_t data[4];
} DATA_DISP32;

typedef struct DATA_IMM64 {
    int8_t data[8];
} DATA_IMM64;

ParseRet parse_disp32() {

}

// ALIAS SIB 1 BYTE= {
//     sibsi 101 = {
//         if Mod == 00 then "{sibsi} + {disp32}"
//         if Mod == 01 then "{sibsi} + {disp8} + [EBP]"
//         if Mod == 10 then "{sibsi} + {disp32} + [EBP]"
//     }
//     sibsi sibb = "{sibsi} + {sibb}"
// }
ParseRet parse_SIB() {
    if (parse_sibsi().success && EXPECT_BITS(0b101)) {
        if (DATA_MOD.value == 0b00) {
            AVAL_SIB= eval_string_00(); // "{sibsi} + {disp32}"
        }
    }
}