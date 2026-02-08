#include "x64.h"

int disassemble(){
}

DATA_OW parse_ow_() {
	uint64_t res= read_bits(&stream, 1);
	return (DATA_OW){._value= res, .parsed= true};
}

bool parse_ow() {
	const DATA_OW res= parse_ow_();
	if (!res.parsed) return false;
	data_ow= res;
	return true;
}

DATA_REGX parse_regx_() {
	uint64_t res= read_bits(&stream, 3);
	return (DATA_REGX){._value= res, .parsed= true};
}

bool parse_regx() {
	const DATA_REGX res= parse_regx_();
	if (!res.parsed) return false;
	data_regx= res;
	return true;
}

DATA_SIBS parse_sibs_() {
	uint64_t res= read_bits(&stream, 2);
	return (DATA_SIBS){._value= res, .parsed= true};
}

bool parse_sibs() {
	const DATA_SIBS res= parse_sibs_();
	if (!res.parsed) return false;
	data_sibs= res;
	return true;
}

DATA_IMM8 parse_imm8_() {
	uint64_t res= read_bits(&stream, 8);
	return (DATA_IMM8){._value= res, .parsed= true};
}

bool parse_imm8() {
	const DATA_IMM8 res= parse_imm8_();
	if (!res.parsed) return false;
	data_imm8= res;
	return true;
}

DATA_IMM16 parse_imm16_() {
	uint64_t res= read_bits(&stream, 16);
	return (DATA_IMM16){._value= res, .parsed= true};
}

bool parse_imm16() {
	const DATA_IMM16 res= parse_imm16_();
	if (!res.parsed) return false;
	data_imm16= res;
	return true;
}

DATA_IMM32 parse_imm32_() {
	uint64_t res= read_bits(&stream, 32);
	return (DATA_IMM32){._value= res, .parsed= true};
}

bool parse_imm32() {
	const DATA_IMM32 res= parse_imm32_();
	if (!res.parsed) return false;
	data_imm32= res;
	return true;
}

DATA_IMM64 parse_imm64_() {
	uint64_t res= read_bits(&stream, 64);
	return (DATA_IMM64){._value= res, .parsed= true};
}

bool parse_imm64() {
	const DATA_IMM64 res= parse_imm64_();
	if (!res.parsed) return false;
	data_imm64= res;
	return true;
}

DATA_DISP8 parse_disp8_() {
	uint64_t res= read_bits(&stream, 8);
	return (DATA_DISP8){._value= res, .parsed= true};
}

bool parse_disp8() {
	const DATA_DISP8 res= parse_disp8_();
	if (!res.parsed) return false;
	data_disp8= res;
	return true;
}

DATA_DISP16 parse_disp16_() {
	uint64_t res= read_bits(&stream, 16);
	return (DATA_DISP16){._value= res, .parsed= true};
}

bool parse_disp16() {
	const DATA_DISP16 res= parse_disp16_();
	if (!res.parsed) return false;
	data_disp16= res;
	return true;
}

DATA_DISP32 parse_disp32_() {
	uint64_t res= read_bits(&stream, 32);
	return (DATA_DISP32){._value= res, .parsed= true};
}

bool parse_disp32() {
	const DATA_DISP32 res= parse_disp32_();
	if (!res.parsed) return false;
	data_disp32= res;
	return true;
}

DATA_REX parse_REX_() {
}

bool parse_REX() {
	const DATA_REX res= parse_REX_();
	if (!res.parsed) return false;
	data_REX= res;
	return true;
}

DATA_VEX parse_VEX_() {
}

bool parse_VEX() {
	const DATA_VEX res= parse_VEX_();
	if (!res.parsed) return false;
	data_VEX= res;
	return true;
}

ParseRet parse_prefix_() {
    if (parse_REX()) return PARSE_SUCC_HIDDEN;
    if (parse_VEX()) return PARSE_SUCC_HIDDEN;
	return PARSE_FAIL; 
}

bool parse_prefix() {
	ParseRet res= parse_prefix_();
	if (!res.success) return res.success;
	aval_prefix= res.aval;
	return res.success;
}

ParseRet parse_lp1_() {
    if (EXPECT_BITS(0xf0)){
        return PARSE_SUCC(to_aval(evaluate_string("LOCK"))); /* LOCK */
    }
    if (EXPECT_BITS(0xf2)){
        return PARSE_SUCC(to_aval(evaluate_string("REPNE"))); /* REPNE */
    }
    if (EXPECT_BITS(0xf3)){
        return PARSE_SUCC(to_aval(evaluate_string("REP"))); /* REP */
    }
	return PARSE_FAIL; 
}

bool parse_lp1() {
	ParseRet res= parse_lp1_();
	if (!res.success) return res.success;
	aval_lp1= res.aval;
	return res.success;
}

ParseRet parse_lp2_() {
    if (EXPECT_BITS(0x2e)){
        return PARSE_SUCC(to_aval(evaluate_string("CS"))); /* CS */
    }
    if (EXPECT_BITS(0x36)){
        return PARSE_SUCC(to_aval(evaluate_string("SS"))); /* SS */
    }
    if (EXPECT_BITS(0x3e)){
        return PARSE_SUCC(to_aval(evaluate_string("DS"))); /* DS */
    }
    if (EXPECT_BITS(0x26)){
        return PARSE_SUCC(to_aval(evaluate_string("ES"))); /* ES */
    }
    if (EXPECT_BITS(0x64)){
        return PARSE_SUCC(to_aval(evaluate_string("FS"))); /* FS */
    }
    if (EXPECT_BITS(0x65)){
        return PARSE_SUCC(to_aval(evaluate_string("GS"))); /* GS */
    }
	return PARSE_FAIL; 
}

bool parse_lp2() {
	ParseRet res= parse_lp2_();
	if (!res.success) return res.success;
	aval_lp2= res.aval;
	return res.success;
}

ParseRet parse_lp3_() {
    if (EXPECT_BITS(0x66)){
        return PARSE_SUCC(to_aval(evaluate_string("OO"))); /* OO */
    }
	return PARSE_FAIL; 
}

bool parse_lp3() {
	ParseRet res= parse_lp3_();
	if (!res.success) return res.success;
	aval_lp3= res.aval;
	return res.success;
}

ParseRet parse_lp4_() {
    if (EXPECT_BITS(0x67)){
        return PARSE_SUCC(to_aval(evaluate_string("AO"))); /* AO */
    }
	return PARSE_FAIL; 
}

bool parse_lp4() {
	ParseRet res= parse_lp4_();
	if (!res.success) return res.success;
	aval_lp4= res.aval;
	return res.success;
}

ParseRet parse_lprefix_() {
    if (parse_lp1()) return PARSE_SUCC_HIDDEN;
    if (parse_lp2()) return PARSE_SUCC_HIDDEN;
    if (parse_lp3()) return PARSE_SUCC_HIDDEN;
    if (parse_lp4()) return PARSE_SUCC_HIDDEN;
	return PARSE_FAIL; 
}

bool parse_lprefix() {
	ParseRet res= parse_lprefix_();
	if (!res.success) return res.success;
	aval_lprefix= res.aval;
	return res.success;
}

ParseRet parse_reg_() {
    if (EXPECT_BITS(0b0000)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("AX"))); /* AX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EAX"))); /* EAX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RAX"))); /* RAX */; break;
        }

    }
    if (EXPECT_BITS(0b1000)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("CX"))); /* CX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ECX"))); /* ECX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RCX"))); /* RCX */; break;
        }

    }
    if (EXPECT_BITS(0b0010)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SP"))); /* SP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AH"))); /* AH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESP"))); /* ESP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSP"))); /* RSP */; break;
        }

    }
    if () return PARSE_SUCC_HIDDEN;
	return PARSE_FAIL; 
}

bool parse_reg() {
	ParseRet res= parse_reg_();
	if (!res.success) return res.success;
	aval_reg= res.aval;
	return res.success;
}

ParseRet parse_regT_() {
    if (EXPECT_BITS(0b000)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("AX"))); /* AX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EAX"))); /* EAX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RAX"))); /* RAX */; break;
        }

    }
    if (EXPECT_BITS(0b100)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("CX"))); /* CX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ECX"))); /* ECX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RCX"))); /* RCX */; break;
        }

    }
    if (EXPECT_BITS(0b001)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SP"))); /* SP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("SPL"))); /* SPL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESP"))); /* ESP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSP"))); /* RSP */; break;
        }

    }
	return PARSE_FAIL; 
}

bool parse_regT() {
	ParseRet res= parse_regT_();
	if (!res.success) return res.success;
	aval_regT= res.aval;
	return res.success;
}

ParseRet parse_cc_() {
    if (EXPECT_BITS(0b0000)){
        return PARSE_SUCC(to_aval(evaluate_string("O"))); /* O */
    }
    if (EXPECT_BITS(0b1000)){
        return PARSE_SUCC(to_aval(evaluate_string("NO"))); /* NO */
    }
    if (EXPECT_BITS(0b0100)){
        return PARSE_SUCC(to_aval(evaluate_string("B"))); /* B */
    }
    if (EXPECT_BITS(0b1100)){
        return PARSE_SUCC(to_aval(evaluate_string("NB"))); /* NB */
    }
    if (EXPECT_BITS(0b0010)){
        return PARSE_SUCC(to_aval(evaluate_string("E"))); /* E */
    }
    if (EXPECT_BITS(0b1010)){
        return PARSE_SUCC(to_aval(evaluate_string("NE"))); /* NE */
    }
    if (EXPECT_BITS(0b0110)){
        return PARSE_SUCC(to_aval(evaluate_string("BE"))); /* BE */
    }
    if (EXPECT_BITS(0b1110)){
        return PARSE_SUCC(to_aval(evaluate_string("NBE"))); /* NBE */
    }
    if (EXPECT_BITS(0b0001)){
        return PARSE_SUCC(to_aval(evaluate_string("S"))); /* S */
    }
    if (EXPECT_BITS(0b1001)){
        return PARSE_SUCC(to_aval(evaluate_string("NS"))); /* NS */
    }
    if (EXPECT_BITS(0b0101)){
        return PARSE_SUCC(to_aval(evaluate_string("P"))); /* P */
    }
    if (EXPECT_BITS(0b1101)){
        return PARSE_SUCC(to_aval(evaluate_string("NP"))); /* NP */
    }
    if (EXPECT_BITS(0b0011)){
        return PARSE_SUCC(to_aval(evaluate_string("L"))); /* L */
    }
    if (EXPECT_BITS(0b1011)){
        return PARSE_SUCC(to_aval(evaluate_string("NL"))); /* NL */
    }
    if (EXPECT_BITS(0b0111)){
        return PARSE_SUCC(to_aval(evaluate_string("LE"))); /* LE */
    }
    if (EXPECT_BITS(0b1111)){
        return PARSE_SUCC(to_aval(evaluate_string("NLE"))); /* NLE */
    }
	return PARSE_FAIL; 
}

bool parse_cc() {
	ParseRet res= parse_cc_();
	if (!res.success) return res.success;
	aval_cc= res.aval;
	return res.success;
}

DATA_REGI parse_regi_() {
	uint64_t res= read_bits(&stream, 3);
	return (DATA_REGI){._value= res, .parsed= true};
}

bool parse_regi() {
	const DATA_REGI res= parse_regi_();
	if (!res.parsed) return false;
	data_regi= res;
	return true;
}

DATA_REGB parse_regb_() {
	uint64_t res= read_bits(&stream, 3);
	return (DATA_REGB){._value= res, .parsed= true};
}

bool parse_regb() {
	const DATA_REGB res= parse_regb_();
	if (!res.parsed) return false;
	data_regb= res;
	return true;
}

DATA_REGR parse_regr_() {
	uint64_t res= read_bits(&stream, 3);
	return (DATA_REGR){._value= res, .parsed= true};
}

bool parse_regr() {
	const DATA_REGR res= parse_regr_();
	if (!res.parsed) return false;
	data_regr= res;
	return true;
}

DATA_MOD parse_Mod_() {
	uint64_t res= read_bits(&stream, 2);
	return (DATA_MOD){._value= res, .parsed= true};
}

bool parse_Mod() {
	const DATA_MOD res= parse_Mod_();
	if (!res.parsed) return false;
	data_Mod= res;
	return true;
}

DATA_RM parse_RM_() {
	uint64_t res= read_bits(&stream, 3);
	return (DATA_RM){._value= res, .parsed= true};
}

bool parse_RM() {
	const DATA_RM res= parse_RM_();
	if (!res.parsed) return false;
	data_RM= res;
	return true;
}

ParseRet parse_sibsi_() {
    if (parse_sibs() && EXPECT_BITS(0b001)){
        return PARSE_SUCC(to_aval(evaluate_string(""))); /*  */
    }
    if (parse_sibs() && parse_regi()){
        return PARSE_SUCC(to_aval(evaluate_string("[{regi} * {2 ^ sibs}]", eval_string_58_field_0, eval_string_58_field_1))); /* [{regi} * {2 ^ sibs}] */
    }
	return PARSE_FAIL; 
}

bool parse_sibsi() {
	ParseRet res= parse_sibsi_();
	if (!res.success) return res.success;
	aval_sibsi= res.aval;
	return res.success;
}

ParseRet parse_SIB_() {
    if (parse_sibsi() && EXPECT_BITS(0b101)){
        if (data_Mod._value == 0b00) {
                return PARSE_SUCC(to_aval(evaluate_string("{sibsi} + {disp32}", eval_string_59_field_0, eval_string_59_field_1))); /* {sibsi} + {disp32} */
            }

if (data_Mod._value == 0b10) {
                return PARSE_SUCC(to_aval(evaluate_string("{sibsi} + {disp8} + [EBP]", eval_string_60_field_0, eval_string_60_field_1))); /* {sibsi} + {disp8} + [EBP] */
            }

if (data_Mod._value == 0b01) {
                return PARSE_SUCC(to_aval(evaluate_string("{sibsi} + {disp32} + [EBP]", eval_string_61_field_0, eval_string_61_field_1))); /* {sibsi} + {disp32} + [EBP] */
            }


    }
    if (parse_sibsi() && parse_regb()){
        return PARSE_SUCC(to_aval(evaluate_string("{regi} + {regb}", eval_string_62_field_0, eval_string_62_field_1))); /* {regi} + {regb} */
    }
	return PARSE_FAIL; 
}

bool parse_SIB() {
	ParseRet res= parse_SIB_();
	if (!res.success) return res.success;
	aval_SIB= res.aval;
	return res.success;
}

ParseRet parse_ModRM_() {
    if (EXPECT_BITS(0b11) && parse_reg() && EXPECT_BITS(0b001) && parse_SIB()){
        return PARSE_SUCC(aval_SIB);
    }
    if (EXPECT_BITS(0b00) && parse_reg() && EXPECT_BITS(0b101) && parse_disp32()){
        if (flag_mode == FLAG_mode_VALUE_64bit) {
                return PARSE_SUCC(to_aval(evaluate_string("rip + {disp32}", eval_string_63_field_0))); /* rip + {disp32} */
            }

            if (parse_disp32()) return PARSE_SUCC_HIDDEN;

    }
    if (EXPECT_BITS(0b00) && parse_reg() && parse_RM()){
        return PARSE_SUCC(to_aval(evaluate_string("[{reg}]", eval_string_64_field_0))); /* [{reg}] */
    }
    if (EXPECT_BITS(0b10) && parse_reg() && parse_RM() && parse_disp8()){
        return PARSE_SUCC(to_aval(evaluate_string("[{reg} + {disp8}]", eval_string_65_field_0, eval_string_65_field_1))); /* [{reg} + {disp8}] */
    }
    if (EXPECT_BITS(0b01) && parse_reg() && parse_RM() && parse_disp32()){
        return PARSE_SUCC(to_aval(evaluate_string("[{reg} + {disp32}]", eval_string_66_field_0, eval_string_66_field_1))); /* [{reg} + {disp32}] */
    }
    if (EXPECT_BITS(0b11) && parse_reg() && parse_RM()){
        return PARSE_SUCC(aval_reg);
    }
	return PARSE_FAIL; 
}

bool parse_ModRM() {
	ParseRet res= parse_ModRM_();
	if (!res.success) return res.success;
	aval_ModRM= res.aval;
	return res.success;
}

ParseRet parse_imm_() {
if (flag_opmode == FLAG_opmode_VALUE_8bit) {
                    if (parse_imm8()){
                return PARSE_SUCC(data_to_aval(data_imm8));
            }

    }

if (flag_opmode == FLAG_opmode_VALUE_16bit) {
                    if (parse_imm16()){
                return PARSE_SUCC(data_to_aval(data_imm16));
            }

    }

if (flag_opmode == FLAG_opmode_VALUE_32bit) {
                    if (parse_imm32()){
                return PARSE_SUCC(data_to_aval(data_imm32));
            }

    }

if (flag_opmode == FLAG_opmode_VALUE_64bit) {
                    if (parse_imm64()){
                return PARSE_SUCC(data_to_aval(data_imm64));
            }

    }

	return PARSE_FAIL; 
}

bool parse_imm() {
	ParseRet res= parse_imm_();
	if (!res.success) return res.success;
	aval_imm= res.aval;
	return res.success;
}

ParseRet parse_immM32_() {
if (flag_opmode == FLAG_opmode_VALUE_8bit) {
                    if (parse_imm8()){
                return PARSE_SUCC(data_to_aval(data_imm8));
            }

    }

if (flag_opmode == FLAG_opmode_VALUE_16bit) {
                    if (parse_imm16()){
                return PARSE_SUCC(data_to_aval(data_imm16));
            }

    }

if (flag_opmode == FLAG_opmode_VALUE_32bit) {
                    if (parse_imm32()){
                return PARSE_SUCC(data_to_aval(data_imm32));
            }

    }

if (flag_opmode == FLAG_opmode_VALUE_64bit) {
                    if (parse_imm32()){
                return PARSE_SUCC(data_to_aval(data_imm32));
            }

    }

	return PARSE_FAIL; 
}

bool parse_immM32() {
	ParseRet res= parse_immM32_();
	if (!res.success) return res.success;
	aval_immM32= res.aval;
	return res.success;
}

ParseRet parse_MOV_() {
    if (EXPECT_BITS(0b0001) && EXPECT_BITS(0b001) && parse_ow() && parse_ModRM()){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {RM}, {regT}", eval_string_67_field_0, eval_string_67_field_1))); /* MOV {RM}, {regT} */
    }
    if (EXPECT_BITS(0b0001) && EXPECT_BITS(0b101) && parse_ow() && parse_ModRM()){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {regT}, {RM}", eval_string_68_field_0, eval_string_68_field_1))); /* MOV {regT}, {RM} */
    }
    if (EXPECT_BITS(0b0011) && EXPECT_BITS(0b110) && parse_ow() && parse_Mod() && EXPECT_BITS(0b000) && parse_RM() && parse_imm()){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {RM}, {imm}", eval_string_69_field_0, eval_string_69_field_1))); /* MOV {RM}, {imm} */
    }
    if (EXPECT_BITS(0b1101) && parse_ow() && parse_reg() && parse_imm()){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {regT}, {imm}", eval_string_70_field_0, eval_string_70_field_1))); /* MOV {regT}, {imm} */
    }
	return PARSE_FAIL; 
}

bool parse_MOV() {
	ParseRet res= parse_MOV_();
	if (!res.success) return res.success;
	aval_MOV= res.aval;
	return res.success;
}

ParseRet parse_ADC_() {
    if (EXPECT_BITS(0b0000) && EXPECT_BITS(0b111) && parse_ow() && parse_immM32()){
        return PARSE_SUCC(to_aval(evaluate_string("ADC RAX, {immM32}", eval_string_71_field_0))); /* ADC RAX, {immM32} */
    }
    if (EXPECT_BITS(0b0001) && EXPECT_BITS(0b000) && parse_ow() && parse_immM32()){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {reg}, {immM32}", eval_string_72_field_0, eval_string_72_field_1))); /* ADC {reg}, {immM32} */
    }
    if (EXPECT_BITS(0b0001) && EXPECT_BITS(0b1100) && parse_Mod() && EXPECT_BITS(0b010) && parse_reg() && parse_imm8()){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {reg}, {imm8}", eval_string_73_field_0, eval_string_73_field_1))); /* ADC {reg}, {imm8} */
    }
    if (EXPECT_BITS(0b1000) && EXPECT_BITS(0b000) && parse_ow() && parse_ModRM()){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {RM}, {reg}", eval_string_74_field_0, eval_string_74_field_1))); /* ADC {RM}, {reg} */
    }
    if (EXPECT_BITS(0b1000) && EXPECT_BITS(0b100) && parse_ow() && parse_ModRM()){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {reg}, {RM}", eval_string_75_field_0, eval_string_75_field_1))); /* ADC {reg}, {RM} */
    }
	return PARSE_FAIL; 
}

bool parse_ADC() {
	ParseRet res= parse_ADC_();
	if (!res.success) return res.success;
	aval_ADC= res.aval;
	return res.success;
}

ParseRet parse_CMOV_() {
    if (EXPECT_BITS(0b0010) && parse_cc() && parse_ModRM()){
        return PARSE_SUCC(to_aval(evaluate_string("CMOV{cc} {reg}, {RM}", eval_string_76_field_0, eval_string_76_field_1, eval_string_76_field_2))); /* CMOV{cc} {reg}, {RM} */
    }
	return PARSE_FAIL; 
}

bool parse_CMOV() {
	ParseRet res= parse_CMOV_();
	if (!res.success) return res.success;
	aval_CMOV= res.aval;
	return res.success;
}

ParseRet parse_op2_() {
    if (EXPECT_BITS(0x1e) && EXPECT_BITS(0xfa)){
        return PARSE_SUCC(to_aval(evaluate_string("ENDBR64"))); /* ENDBR64 */
    }
    if (parse_CMOV()) return PARSE_SUCC_HIDDEN;
	return PARSE_FAIL; 
}

bool parse_op2() {
	ParseRet res= parse_op2_();
	if (!res.success) return res.success;
	aval_op2= res.aval;
	return res.success;
}

ParseRet parse_op_() {
    if (EXPECT_BITS(0xf) && parse_op2()) return PARSE_SUCC_HIDDEN;
    if (parse_MOV()) return PARSE_SUCC_HIDDEN;
    if (parse_ADC()) return PARSE_SUCC_HIDDEN;
	return PARSE_FAIL; 
}

bool parse_op() {
	ParseRet res= parse_op_();
	if (!res.success) return res.success;
	aval_op= res.aval;
	return res.success;
}

ParseRet parse_reg_a_() {
    if (parse_reg()){
        return PARSE_SUCC(aval_reg);
    }
	return PARSE_FAIL; 
}

bool parse_reg_a() {
	ParseRet res= parse_reg_a_();
	if (!res.success) return res.success;
	aval_reg_a= res.aval;
	return res.success;
}

ParseRet parse_reg_b_() {
    if (parse_reg()){
        return PARSE_SUCC(aval_reg);
    }
	return PARSE_FAIL; 
}

bool parse_reg_b() {
	ParseRet res= parse_reg_b_();
	if (!res.success) return res.success;
	aval_reg_b= res.aval;
	return res.success;
}

ParseRet parse_name_() {
    if (EXPECT_BITS(0xff) && EXPECT_BITS(0b1101) && EXPECT_BITS(0b100) && parse_ow() && parse_reg_a() && parse_reg_b()){
        return PARSE_SUCC(to_aval(evaluate_string("Hello world {reg_a}*{ow} {reg_b}", eval_string_78_field_0, eval_string_78_field_1, eval_string_78_field_2))); /* Hello world {reg_a}*{ow} {reg_b} */
    }
	return PARSE_FAIL; 
}

bool parse_name() {
	ParseRet res= parse_name_();
	if (!res.success) return res.success;
	aval_name= res.aval;
	return res.success;
}

int init() {
	aval_prefix.choices= vector_create();
	aval_lp1.choices= vector_create();
	aval_lp2.choices= vector_create();
	aval_lp3.choices= vector_create();
	aval_lp4.choices= vector_create();
	aval_lprefix.choices= vector_create();
	aval_reg.choices= vector_create();
	aval_regT.choices= vector_create();
	aval_cc.choices= vector_create();
	aval_sibsi.choices= vector_create();
	aval_SIB.choices= vector_create();
	aval_ModRM.choices= vector_create();
	aval_imm.choices= vector_create();
	aval_immM32.choices= vector_create();
	aval_MOV.choices= vector_create();
	aval_ADC.choices= vector_create();
	aval_CMOV.choices= vector_create();
	aval_op2.choices= vector_create();
	aval_op.choices= vector_create();
	aval_reg_a.choices= vector_create();
	aval_reg_b.choices= vector_create();
	aval_name.choices= vector_create();
	return 0;
}

