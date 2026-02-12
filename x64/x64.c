#include "x64.h"

int disassemble(const char** output){
	reset();
	while (parse_lprefix(&top_stream)) {}

	parse_prefix(&top_stream);

	if (!parse_op(&top_stream)) return false;
	*output= get_aval_op().chosen_val;
	return true;
}

void reset() {
	flag_mode= FLAG_MODE_VALUE_64bit;
	flag_calculated_mode= false;
	flag_opmode= FLAG_OPMODE_VALUE_32bit;
	flag_calculated_opmode= false;
	flag_addrmode= FLAG_ADDRMODE_VALUE_64bit;
	flag_calculated_addrmode= false;
	data_ow.parsed= false;
	data_ow._value= 0;
	data_regx.parsed= false;
	data_regx._value= 0;
	data_sibs.parsed= false;
	data_sibs._value= 0;
	data_imm8.parsed= false;
	data_imm8._value= 0;
	data_imm16.parsed= false;
	data_imm16._value= 0;
	data_imm32.parsed= false;
	data_imm32._value= 0;
	data_imm64.parsed= false;
	data_imm64._value= 0;
	data_disp8.parsed= false;
	data_disp8._value= 0;
	data_disp16.parsed= false;
	data_disp16._value= 0;
	data_disp32.parsed= false;
	data_disp32._value= 0;
	data_REX.parsed= false;
	data_REX= (DATA_REX){
		.w= 0b0,
	};
	data_VEX.parsed= false;
	data_VEX= (DATA_VEX){
		0
	};
	clear_aval(&aval_prefix);
	clear_aval(&aval_lp1);
	clear_aval(&aval_lp2);
	clear_aval(&aval_lp3);
	clear_aval(&aval_lp4);
	clear_aval(&aval_lprefix);
	clear_aval(&aval_reg);
	clear_aval(&aval_regT);
	clear_aval(&aval_regO);
	clear_aval(&aval_regM);
	clear_aval(&aval_cc);
	clear_aval(&aval_regi);
	clear_aval(&aval_regb);
	clear_aval(&aval_regop);
	clear_aval(&aval_regr);
	data_Mod.parsed= false;
	data_Mod._value= 0;
	clear_aval(&aval_addr_ptr);
	clear_aval(&aval_sibsi);
	clear_aval(&aval_SIB_INT);
	clear_aval(&aval_SIB);
	clear_aval(&aval_rm_INT);
	clear_aval(&aval_rm);
	clear_aval(&aval_rm_ptr);
	clear_aval(&aval_ModRM);
	clear_aval(&aval_imm);
	clear_aval(&aval_immM32);
	data_ms.parsed= false;
	data_ms._value= 0;
	clear_aval(&aval_ModRMS);
	clear_aval(&aval_MOV);
	clear_aval(&aval_regA);
	clear_aval(&aval_ADC);
	clear_aval(&aval_PUSH);
	clear_aval(&aval_CMOV);
	clear_aval(&aval_ModRM_5);
	clear_aval(&aval_ModRM_2);
	clear_aval(&aval_SUB);
	clear_aval(&aval_op2);
	clear_aval(&aval_op);
}

void eval_string_207_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regi().chosen_val);
}

void eval_string_207_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(pow(0x2, data_sibs._value)));
}

void eval_string_208_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_sibsi().chosen_val);
}

void eval_string_208_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_disp32._value));
}

void eval_string_209_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_sibsi().chosen_val);
}

void eval_string_209_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_disp8._value));
}

void eval_string_210_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_sibsi().chosen_val);
}

void eval_string_210_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_disp32._value));
}

void eval_string_211_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regb().chosen_val);
}

void eval_string_211_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_sibsi().chosen_val);
}

void eval_string_212_field_0(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_disp32._value));
}

void eval_string_213_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regb().chosen_val);
}

void eval_string_213_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_disp8._value));
}

void eval_string_214_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regb().chosen_val);
}

void eval_string_214_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_disp32._value));
}

void eval_string_215_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_rm_INT().chosen_val);
}

void eval_string_216_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_addr_ptr().chosen_val);
}

void eval_string_216_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_rm().chosen_val);
}

void eval_string_217_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regr().chosen_val);
}

void eval_string_217_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_rm().chosen_val);
}

void eval_string_218_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_rm_ptr().chosen_val);
}

void eval_string_218_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_regr().chosen_val);
}

void eval_string_219_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regr().chosen_val);
}

void eval_string_219_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_rm().chosen_val);
}

void eval_string_220_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_ModRMS().chosen_val);
}

void eval_string_221_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_rm_ptr().chosen_val);
}

void eval_string_221_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_immM32().chosen_val);
}

void eval_string_222_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regT().chosen_val);
}

void eval_string_222_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_imm().chosen_val);
}

void eval_string_223_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_immM32().chosen_val);
}

void eval_string_224_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_reg().chosen_val);
}

void eval_string_224_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_immM32().chosen_val);
}

void eval_string_225_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_reg().chosen_val);
}

void eval_string_225_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_imm8._value));
}

void eval_string_226_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_ModRMS().chosen_val);
}

void eval_string_227_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_rm().chosen_val);
}

void eval_string_228_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regop().chosen_val);
}

void eval_string_229_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_immM32().chosen_val);
}

void eval_string_230_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_cc().chosen_val);
}

void eval_string_230_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_ModRM().chosen_val);
}

void eval_string_231_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_regA().chosen_val);
}

void eval_string_231_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_immM32().chosen_val);
}

void eval_string_232_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_ModRM_5().chosen_val);
}

void eval_string_232_field_1(Buffer* buff) {
	buffer_concat(buff, get_aval_immM32().chosen_val);
}

void eval_string_233_field_0(Buffer* buff) {
	buffer_concat(buff, get_aval_ModRM_5().chosen_val);
}

void eval_string_233_field_1(Buffer* buff) {
	buffer_concat(buff, data_to_string(data_imm8._value));
}

ParseRet parse_with_0(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0xff, stream) && parse_Mod(stream) && EXPECT_BITS(0b110, stream) && parse_rm(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("PUSH {rm}", eval_string_227_field_0))); /* PUSH {rm} */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b0101, stream) && EXPECT_BITS(0b0, stream) && parse_regop(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("PUSH {regop}", eval_string_228_field_0))); /* PUSH {regop} */
    }
    stream->pointer= pos_save_1;

return PARSE_FAIL;
}

FLAG_MODE get_flag_mode() {
	if (flag_calculated_mode) return flag_mode;
	flag_calculated_mode= true;
	return flag_mode;
}

FLAG_OPMODE get_flag_opmode() {
	if (flag_calculated_opmode) return flag_opmode;
	calculate_flag_opmode();
	flag_calculated_opmode= true;
	return flag_opmode;
}

FLAG_ADDRMODE get_flag_addrmode() {
	if (flag_calculated_addrmode) return flag_addrmode;
	calculate_flag_addrmode();
	flag_calculated_addrmode= true;
	return flag_addrmode;
}

DATA_OW parse_ow_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 1);
	return (DATA_OW){._value= res, .parsed= true};
}

bool parse_ow(ByteStream* stream) {
	const DATA_OW res= parse_ow_(stream);
	if (!res.parsed) return false;
	data_ow= res;
	return true;
}

DATA_REGX parse_regx_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 3);
	return (DATA_REGX){._value= res, .parsed= true};
}

bool parse_regx(ByteStream* stream) {
	const DATA_REGX res= parse_regx_(stream);
	if (!res.parsed) return false;
	data_regx= res;
	return true;
}

DATA_SIBS parse_sibs_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 2);
	return (DATA_SIBS){._value= res, .parsed= true};
}

bool parse_sibs(ByteStream* stream) {
	const DATA_SIBS res= parse_sibs_(stream);
	if (!res.parsed) return false;
	data_sibs= res;
	return true;
}

DATA_IMM8 parse_imm8_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 8);
	return (DATA_IMM8){._value= res, .parsed= true};
}

bool parse_imm8(ByteStream* stream) {
	const DATA_IMM8 res= parse_imm8_(stream);
	if (!res.parsed) return false;
	data_imm8= res;
	return true;
}

DATA_IMM16 parse_imm16_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 16);
	return (DATA_IMM16){._value= res, .parsed= true};
}

bool parse_imm16(ByteStream* stream) {
	const DATA_IMM16 res= parse_imm16_(stream);
	if (!res.parsed) return false;
	data_imm16= res;
	return true;
}

DATA_IMM32 parse_imm32_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 32);
	return (DATA_IMM32){._value= res, .parsed= true};
}

bool parse_imm32(ByteStream* stream) {
	const DATA_IMM32 res= parse_imm32_(stream);
	if (!res.parsed) return false;
	data_imm32= res;
	return true;
}

DATA_IMM64 parse_imm64_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 64);
	return (DATA_IMM64){._value= res, .parsed= true};
}

bool parse_imm64(ByteStream* stream) {
	const DATA_IMM64 res= parse_imm64_(stream);
	if (!res.parsed) return false;
	data_imm64= res;
	return true;
}

DATA_DISP8 parse_disp8_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 8);
	return (DATA_DISP8){._value= res, .parsed= true};
}

bool parse_disp8(ByteStream* stream) {
	const DATA_DISP8 res= parse_disp8_(stream);
	if (!res.parsed) return false;
	data_disp8= res;
	return true;
}

DATA_DISP16 parse_disp16_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 16);
	return (DATA_DISP16){._value= res, .parsed= true};
}

bool parse_disp16(ByteStream* stream) {
	const DATA_DISP16 res= parse_disp16_(stream);
	if (!res.parsed) return false;
	data_disp16= res;
	return true;
}

DATA_DISP32 parse_disp32_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 32);
	return (DATA_DISP32){._value= res, .parsed= true};
}

bool parse_disp32(ByteStream* stream) {
	const DATA_DISP32 res= parse_disp32_(stream);
	if (!res.parsed) return false;
	data_disp32= res;
	return true;
}

DATA_REX parse_REX_(ByteStream* stream) {
	DATA_REX res;

	const size_t pos_save0= stream->pointer;

	if (EXPECT_BITS(0b0100, stream) &&
		(res.w= read_bits(stream, 1), true) &&
		(res.r= read_bits(stream, 1), true) &&
		(res.x= read_bits(stream, 1), true) &&
		(res.b= read_bits(stream, 1), true)
	) {
		res.parsed= true;
		return res;
	}

	stream->pointer= pos_save0;

	res.parsed= false;
	return res;
}

bool parse_REX(ByteStream* stream) {
	const DATA_REX res= parse_REX_(stream);
	if (!res.parsed) return false;
	data_REX= res;
	return true;
}

DATA_VEX parse_VEX_(ByteStream* stream) {
	DATA_VEX res;

	const size_t pos_save0= stream->pointer;

	if (EXPECT_BITS(0b1100, stream) &&
		EXPECT_BITS(0b0100, stream) &&
		(res.R= read_bits(stream, 1), true) &&
		(res.X= read_bits(stream, 1), true) &&
		(res.B= read_bits(stream, 1), true) &&
		(res.m= read_bits(stream, 5), true) &&
		(res.W= read_bits(stream, 1), true) &&
		(res.v= read_bits(stream, 4), true) &&
		(res.L= read_bits(stream, 1), true) &&
		(res.pp= read_bits(stream, 2), true)
	) {
		res.parsed= true;
		return res;
	}

	stream->pointer= pos_save0;

	const size_t pos_save1= stream->pointer;

	if (EXPECT_BITS(0b1100, stream) &&
		EXPECT_BITS(0b0101, stream) &&
		(res.R= read_bits(stream, 1), true) &&
		(res.v= read_bits(stream, 4), true) &&
		(res.L= read_bits(stream, 1), true) &&
		(res.pp= read_bits(stream, 2), true)
	) {
		res.parsed= true;
		return res;
	}

	stream->pointer= pos_save1;

	res.parsed= false;
	return res;
}

bool parse_VEX(ByteStream* stream) {
	const DATA_VEX res= parse_VEX_(stream);
	if (!res.parsed) return false;
	data_VEX= res;
	return true;
}

ParseRet parse_prefix_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_REX(stream)) return PARSE_SUCC_HIDDEN;;
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_VEX(stream)) return PARSE_SUCC_HIDDEN;;
    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_prefix(ByteStream* stream) {
	ParseRet res= parse_prefix_(stream);
	if (!res.success) return res.success;
	aval_prefix= res.aval;
	return res.success;
}

AVAL get_aval_prefix() {
	return aval_prefix;
}

ParseRet parse_lp1_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0xf0, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("LOCK"))); /* LOCK */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BYTE(0xf2, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("REPNE"))); /* REPNE */
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BYTE(0xf3, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("REP"))); /* REP */
    }
    stream->pointer= pos_save_2;

	return PARSE_FAIL; 
}

bool parse_lp1(ByteStream* stream) {
	ParseRet res= parse_lp1_(stream);
	if (!res.success) return res.success;
	aval_lp1= res.aval;
	return res.success;
}

AVAL get_aval_lp1() {
	return aval_lp1;
}

ParseRet parse_lp2_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0x2e, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("CS"))); /* CS */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BYTE(0x36, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("SS"))); /* SS */
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BYTE(0x3e, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("DS"))); /* DS */
    }
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (EXPECT_BYTE(0x26, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("ES"))); /* ES */
    }
    stream->pointer= pos_save_3;

    size_t pos_save_4= stream->pointer;

    if (EXPECT_BYTE(0x64, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("FS"))); /* FS */
    }
    stream->pointer= pos_save_4;

    size_t pos_save_5= stream->pointer;

    if (EXPECT_BYTE(0x65, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("GS"))); /* GS */
    }
    stream->pointer= pos_save_5;

	return PARSE_FAIL; 
}

bool parse_lp2(ByteStream* stream) {
	ParseRet res= parse_lp2_(stream);
	if (!res.success) return res.success;
	aval_lp2= res.aval;
	return res.success;
}

AVAL get_aval_lp2() {
	return aval_lp2;
}

ParseRet parse_lp3_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0x66, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("OO"))); /* OO */
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_lp3(ByteStream* stream) {
	ParseRet res= parse_lp3_(stream);
	if (!res.success) return res.success;
	aval_lp3= res.aval;
	return res.success;
}

AVAL get_aval_lp3() {
	return aval_lp3;
}

ParseRet parse_lp4_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0x67, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("AO"))); /* AO */
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_lp4(ByteStream* stream) {
	ParseRet res= parse_lp4_(stream);
	if (!res.success) return res.success;
	aval_lp4= res.aval;
	return res.success;
}

AVAL get_aval_lp4() {
	return aval_lp4;
}

ParseRet parse_lprefix_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_lp1(stream)) return PARSE_SUCC(get_aval_lp1());
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_lp2(stream)) return PARSE_SUCC(get_aval_lp2());
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (parse_lp3(stream)) return PARSE_SUCC(get_aval_lp3());
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (parse_lp4(stream)) return PARSE_SUCC(get_aval_lp4());
    stream->pointer= pos_save_3;

	return PARSE_FAIL; 
}

bool parse_lprefix(ByteStream* stream) {
	ParseRet res= parse_lprefix_(stream);
	if (!res.success) return res.success;
	aval_lprefix= res.aval;
	return res.success;
}

AVAL get_aval_lprefix() {
	return aval_lprefix;
}

ParseRet parse_reg_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b0000, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("AX"))); /* AX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EAX"))); /* EAX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RAX"))); /* RAX */; break;
        }
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b0001, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("CX"))); /* CX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ECX"))); /* ECX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RCX"))); /* RCX */; break;
        }
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b0010, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("DX"))); /* DX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("DL"))); /* DL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("DL"))); /* DL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EDX"))); /* EDX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RDX"))); /* RDX */; break;
        }
    }
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (EXPECT_BITS(0b0011, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("BX"))); /* BX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("BL"))); /* BL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("BL"))); /* BL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EBX"))); /* EBX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RBX"))); /* RBX */; break;
        }
    }
    stream->pointer= pos_save_3;

    size_t pos_save_4= stream->pointer;

    if (EXPECT_BITS(0b0100, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SP"))); /* SP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AH"))); /* AH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AH"))); /* AH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESP"))); /* ESP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSP"))); /* RSP */; break;
        }
    }
    stream->pointer= pos_save_4;

    size_t pos_save_5= stream->pointer;

    if (EXPECT_BITS(0b0101, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("BP"))); /* BP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CH"))); /* CH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("CH"))); /* CH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EBP"))); /* EBP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RBP"))); /* RBP */; break;
        }
    }
    stream->pointer= pos_save_5;

    size_t pos_save_6= stream->pointer;

    if (EXPECT_BITS(0b0110, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SI"))); /* SI */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("DH"))); /* DH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("DH"))); /* DH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESI"))); /* ESI */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSI"))); /* RSI */; break;
        }
    }
    stream->pointer= pos_save_6;

    size_t pos_save_7= stream->pointer;

    if (EXPECT_BITS(0b0111, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("DI"))); /* DI */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("BH"))); /* BH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("BH"))); /* BH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EDI"))); /* EDI */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RDI"))); /* RDI */; break;
        }
    }
    stream->pointer= pos_save_7;

    size_t pos_save_8= stream->pointer;

    if (EXPECT_BITS(0b1000, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R8W"))); /* R8W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R8B"))); /* R8B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R8B"))); /* R8B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R8D"))); /* R8D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R8"))); /* R8 */; break;
        }
    }
    stream->pointer= pos_save_8;

    size_t pos_save_9= stream->pointer;

    if (EXPECT_BITS(0b1001, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R9W"))); /* R9W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R9B"))); /* R9B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R9B"))); /* R9B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R9D"))); /* R9D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R9"))); /* R9 */; break;
        }
    }
    stream->pointer= pos_save_9;

    size_t pos_save_10= stream->pointer;

    if (EXPECT_BITS(0b1010, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R10W"))); /* R10W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R10B"))); /* R10B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R10B"))); /* R10B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R10D"))); /* R10D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R10"))); /* R10 */; break;
        }
    }
    stream->pointer= pos_save_10;

    size_t pos_save_11= stream->pointer;

    if (EXPECT_BITS(0b1011, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R11W"))); /* R11W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R11B"))); /* R11B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R11B"))); /* R11B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R11D"))); /* R11D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R11"))); /* R11 */; break;
        }
    }
    stream->pointer= pos_save_11;

    size_t pos_save_12= stream->pointer;

    if (EXPECT_BITS(0b1100, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R12W"))); /* R12W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R12B"))); /* R12B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R12B"))); /* R12B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R12D"))); /* R12D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R12"))); /* R12 */; break;
        }
    }
    stream->pointer= pos_save_12;

    size_t pos_save_13= stream->pointer;

    if (EXPECT_BITS(0b1101, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R13W"))); /* R13W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R13B"))); /* R13B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R13B"))); /* R13B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R13D"))); /* R13D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R13"))); /* R13 */; break;
        }
    }
    stream->pointer= pos_save_13;

    size_t pos_save_14= stream->pointer;

    if (EXPECT_BITS(0b1110, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R14W"))); /* R14W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R14B"))); /* R14B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R14B"))); /* R14B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R14D"))); /* R14D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R14"))); /* R14 */; break;
        }
    }
    stream->pointer= pos_save_14;

    size_t pos_save_15= stream->pointer;

    if (EXPECT_BITS(0b1111, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R15W"))); /* R15W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R15B"))); /* R15B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R15B"))); /* R15B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R15D"))); /* R15D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R15"))); /* R15 */; break;
        }
    }
    stream->pointer= pos_save_15;

	return PARSE_FAIL; 
}

bool parse_reg(ByteStream* stream) {
	ParseRet res= parse_reg_(stream);
	if (!res.success) return res.success;
	aval_reg= res.aval;
	return res.success;
}

AVAL get_aval_reg() {
	return aval_reg;
}

ParseRet parse_regT_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b000, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("AX"))); /* AX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EAX"))); /* EAX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RAX"))); /* RAX */; break;
        }
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b001, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("CX"))); /* CX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ECX"))); /* ECX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RCX"))); /* RCX */; break;
        }
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b100, stream)){
        switch(calculate_rule_right_0()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SP"))); /* SP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("SPL"))); /* SPL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESP"))); /* ESP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSP"))); /* RSP */; break;
        }
    }
    stream->pointer= pos_save_2;

	return PARSE_FAIL; 
}

bool parse_regT(ByteStream* stream) {
	ParseRet res= parse_regT_(stream);
	if (!res.success) return res.success;
	aval_regT= res.aval;
	return res.success;
}

AVAL get_aval_regT() {
	return aval_regT;
}

ParseRet parse_regO_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_reg(stream)){
        return PARSE_SUCC(get_aval_reg());
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_regO(ByteStream* stream) {
	ParseRet res= parse_regO_(stream);
	if (!res.success) return res.success;
	aval_regO= res.aval;
	return res.success;
}

AVAL get_aval_regO() {
	return aval_regO;
}

ParseRet parse_regM_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b0000, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("AX"))); /* AX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AL"))); /* AL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EAX"))); /* EAX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RAX"))); /* RAX */; break;
        }
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b0001, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("CX"))); /* CX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("CL"))); /* CL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ECX"))); /* ECX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RCX"))); /* RCX */; break;
        }
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b0010, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("DX"))); /* DX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("DL"))); /* DL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("DL"))); /* DL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EDX"))); /* EDX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RDX"))); /* RDX */; break;
        }
    }
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (EXPECT_BITS(0b0011, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("BX"))); /* BX */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("BL"))); /* BL */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("BL"))); /* BL */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EBX"))); /* EBX */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RBX"))); /* RBX */; break;
        }
    }
    stream->pointer= pos_save_3;

    size_t pos_save_4= stream->pointer;

    if (EXPECT_BITS(0b0100, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SP"))); /* SP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("AH"))); /* AH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("AH"))); /* AH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESP"))); /* ESP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSP"))); /* RSP */; break;
        }
    }
    stream->pointer= pos_save_4;

    size_t pos_save_5= stream->pointer;

    if (EXPECT_BITS(0b0101, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("BP"))); /* BP */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("CH"))); /* CH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("CH"))); /* CH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EBP"))); /* EBP */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RBP"))); /* RBP */; break;
        }
    }
    stream->pointer= pos_save_5;

    size_t pos_save_6= stream->pointer;

    if (EXPECT_BITS(0b0110, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("SI"))); /* SI */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("DH"))); /* DH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("DH"))); /* DH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("ESI"))); /* ESI */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RSI"))); /* RSI */; break;
        }
    }
    stream->pointer= pos_save_6;

    size_t pos_save_7= stream->pointer;

    if (EXPECT_BITS(0b0111, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("DI"))); /* DI */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("BH"))); /* BH */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("BH"))); /* BH */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("EDI"))); /* EDI */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("RDI"))); /* RDI */; break;
        }
    }
    stream->pointer= pos_save_7;

    size_t pos_save_8= stream->pointer;

    if (EXPECT_BITS(0b1000, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R8W"))); /* R8W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R8B"))); /* R8B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R8B"))); /* R8B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R8D"))); /* R8D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R8"))); /* R8 */; break;
        }
    }
    stream->pointer= pos_save_8;

    size_t pos_save_9= stream->pointer;

    if (EXPECT_BITS(0b1001, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R9W"))); /* R9W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R9B"))); /* R9B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R9B"))); /* R9B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R9D"))); /* R9D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R9"))); /* R9 */; break;
        }
    }
    stream->pointer= pos_save_9;

    size_t pos_save_10= stream->pointer;

    if (EXPECT_BITS(0b1010, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R10W"))); /* R10W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R10B"))); /* R10B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R10B"))); /* R10B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R10D"))); /* R10D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R10"))); /* R10 */; break;
        }
    }
    stream->pointer= pos_save_10;

    size_t pos_save_11= stream->pointer;

    if (EXPECT_BITS(0b1011, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R11W"))); /* R11W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R11B"))); /* R11B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R11B"))); /* R11B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R11D"))); /* R11D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R11"))); /* R11 */; break;
        }
    }
    stream->pointer= pos_save_11;

    size_t pos_save_12= stream->pointer;

    if (EXPECT_BITS(0b1100, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R12W"))); /* R12W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R12B"))); /* R12B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R12B"))); /* R12B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R12D"))); /* R12D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R12"))); /* R12 */; break;
        }
    }
    stream->pointer= pos_save_12;

    size_t pos_save_13= stream->pointer;

    if (EXPECT_BITS(0b1101, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R13W"))); /* R13W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R13B"))); /* R13B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R13B"))); /* R13B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R13D"))); /* R13D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R13"))); /* R13 */; break;
        }
    }
    stream->pointer= pos_save_13;

    size_t pos_save_14= stream->pointer;

    if (EXPECT_BITS(0b1110, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R14W"))); /* R14W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R14B"))); /* R14B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R14B"))); /* R14B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R14D"))); /* R14D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R14"))); /* R14 */; break;
        }
    }
    stream->pointer= pos_save_14;

    size_t pos_save_15= stream->pointer;

    if (EXPECT_BITS(0b1111, stream)){
        switch(calculate_rule_right_1()) {
            case 0: return PARSE_SUCC(to_aval(evaluate_string("R15W"))); /* R15W */; break;
            case 1: return PARSE_SUCC(to_aval(evaluate_string("R15B"))); /* R15B */; break;
            case 2: return PARSE_SUCC(to_aval(evaluate_string("R15B"))); /* R15B */; break;
            case 3: return PARSE_SUCC(to_aval(evaluate_string("R15D"))); /* R15D */; break;
            case 4: return PARSE_SUCC(to_aval(evaluate_string("R15"))); /* R15 */; break;
        }
    }
    stream->pointer= pos_save_15;

	return PARSE_FAIL; 
}

bool parse_regM(ByteStream* stream) {
	ParseRet res= parse_regM_(stream);
	if (!res.success) return res.success;
	aval_regM= res.aval;
	return res.success;
}

AVAL get_aval_regM() {
	return aval_regM;
}

int calculate_rule_right_0() {
	uint16_t choice= 0;

	if (get_flag_opmode() == FLAG_OPMODE_VALUE_16bit) choice= 0;
	if (get_flag_opmode() == FLAG_OPMODE_VALUE_8bit && data_REX.w != 0b1) choice= 1;
	if (get_flag_opmode() == FLAG_OPMODE_VALUE_8bit) choice= 2;
	if (get_flag_opmode() == FLAG_OPMODE_VALUE_32bit) choice= 3;
	if (get_flag_opmode() == FLAG_OPMODE_VALUE_64bit) choice= 4;
	return choice;
}

int calculate_rule_right_1() {
	uint16_t choice= 0;

	if (get_flag_addrmode() == FLAG_ADDRMODE_VALUE_16bit) choice= 0;
	if (0b0) choice= 1;
	if (0b0) choice= 2;
	if (get_flag_addrmode() == FLAG_ADDRMODE_VALUE_32bit) choice= 3;
	if (get_flag_addrmode() == FLAG_ADDRMODE_VALUE_64bit) choice= 4;
	return choice;
}

ParseRet parse_cc_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b0000, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("O"))); /* O */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b0001, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NO"))); /* NO */
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b0010, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("B"))); /* B */
    }
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (EXPECT_BITS(0b0011, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NB"))); /* NB */
    }
    stream->pointer= pos_save_3;

    size_t pos_save_4= stream->pointer;

    if (EXPECT_BITS(0b0100, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("E"))); /* E */
    }
    stream->pointer= pos_save_4;

    size_t pos_save_5= stream->pointer;

    if (EXPECT_BITS(0b0101, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NE"))); /* NE */
    }
    stream->pointer= pos_save_5;

    size_t pos_save_6= stream->pointer;

    if (EXPECT_BITS(0b0110, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("BE"))); /* BE */
    }
    stream->pointer= pos_save_6;

    size_t pos_save_7= stream->pointer;

    if (EXPECT_BITS(0b0111, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NBE"))); /* NBE */
    }
    stream->pointer= pos_save_7;

    size_t pos_save_8= stream->pointer;

    if (EXPECT_BITS(0b1000, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("S"))); /* S */
    }
    stream->pointer= pos_save_8;

    size_t pos_save_9= stream->pointer;

    if (EXPECT_BITS(0b1001, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NS"))); /* NS */
    }
    stream->pointer= pos_save_9;

    size_t pos_save_10= stream->pointer;

    if (EXPECT_BITS(0b1010, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("P"))); /* P */
    }
    stream->pointer= pos_save_10;

    size_t pos_save_11= stream->pointer;

    if (EXPECT_BITS(0b1011, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NP"))); /* NP */
    }
    stream->pointer= pos_save_11;

    size_t pos_save_12= stream->pointer;

    if (EXPECT_BITS(0b1100, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("L"))); /* L */
    }
    stream->pointer= pos_save_12;

    size_t pos_save_13= stream->pointer;

    if (EXPECT_BITS(0b1101, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NL"))); /* NL */
    }
    stream->pointer= pos_save_13;

    size_t pos_save_14= stream->pointer;

    if (EXPECT_BITS(0b1110, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("LE"))); /* LE */
    }
    stream->pointer= pos_save_14;

    size_t pos_save_15= stream->pointer;

    if (EXPECT_BITS(0b1111, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("NLE"))); /* NLE */
    }
    stream->pointer= pos_save_15;

	return PARSE_FAIL; 
}

bool parse_cc(ByteStream* stream) {
	ParseRet res= parse_cc_(stream);
	if (!res.success) return res.success;
	aval_cc= res.aval;
	return res.success;
}

AVAL get_aval_cc() {
	return aval_cc;
}

ParseRet parse_regi_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_regx(stream)){
        ByteStream stream= stream_create();
        stream_add(&stream, data_REX.x, 1);
        stream_add(&stream, data_regx._value, 3);
        ParseRet res= parse_regM_(&stream);
        stream_destroy(&stream);
        return res;
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_regi(ByteStream* stream) {
	ParseRet res= parse_regi_(stream);
	if (!res.success) return res.success;
	aval_regi= res.aval;
	return res.success;
}

AVAL get_aval_regi() {
	return aval_regi;
}

ParseRet parse_regb_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_regx(stream)){
        ByteStream stream= stream_create();
        stream_add(&stream, data_REX.b, 1);
        stream_add(&stream, data_regx._value, 3);
        ParseRet res= parse_regM_(&stream);
        stream_destroy(&stream);
        return res;
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_regb(ByteStream* stream) {
	ParseRet res= parse_regb_(stream);
	if (!res.success) return res.success;
	aval_regb= res.aval;
	return res.success;
}

AVAL get_aval_regb() {
	return aval_regb;
}

ParseRet parse_regop_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_regx(stream)){
        ByteStream stream= stream_create();
        stream_add(&stream, data_REX.b, 1);
        stream_add(&stream, data_regx._value, 3);
        ParseRet res= parse_regO_(&stream);
        stream_destroy(&stream);
        return res;
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_regop(ByteStream* stream) {
	ParseRet res= parse_regop_(stream);
	if (!res.success) return res.success;
	aval_regop= res.aval;
	return res.success;
}

AVAL get_aval_regop() {
	return aval_regop;
}

ParseRet parse_regr_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_regx(stream)){
        ByteStream stream= stream_create();
        stream_add(&stream, data_REX.r, 1);
        stream_add(&stream, data_regx._value, 3);
        ParseRet res= parse_regO_(&stream);
        stream_destroy(&stream);
        return res;
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_regr(ByteStream* stream) {
	ParseRet res= parse_regr_(stream);
	if (!res.success) return res.success;
	aval_regr= res.aval;
	return res.success;
}

AVAL get_aval_regr() {
	return aval_regr;
}

DATA_MOD parse_Mod_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 2);
	return (DATA_MOD){._value= res, .parsed= true};
}

bool parse_Mod(ByteStream* stream) {
	const DATA_MOD res= parse_Mod_(stream);
	if (!res.parsed) return false;
	data_Mod= res;
	return true;
}

void calculate_flag_addrmode() {
    if (aval_lp4.parsed_successfully){
        flag_addrmode= FLAG_ADDRMODE_VALUE_32bit; return;
    }
            flag_addrmode= FLAG_ADDRMODE_VALUE_64bit; return;
}
ParseRet parse_addr_ptr_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_64bit) {
    return PARSE_SUCC(to_aval(evaluate_string("QWORD PTR"))); /* QWORD PTR */
    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_32bit) {
    return PARSE_SUCC(to_aval(evaluate_string("DWORD PTR"))); /* DWORD PTR */
    }

    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_16bit) {
    return PARSE_SUCC(to_aval(evaluate_string("WORD PTR"))); /* WORD PTR */
    }

    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_8bit) {
    return PARSE_SUCC(to_aval(evaluate_string("BYTE PTR"))); /* BYTE PTR */
    }

    stream->pointer= pos_save_3;

	return PARSE_FAIL; 
}

bool parse_addr_ptr(ByteStream* stream) {
	ParseRet res= parse_addr_ptr_(stream);
	if (!res.success) return res.success;
	aval_addr_ptr= res.aval;
	return res.success;
}

AVAL get_aval_addr_ptr() {
	parse_addr_ptr(&top_stream);
	return aval_addr_ptr;
}

ParseRet parse_sibsi_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_sibs(stream) && EXPECT_BITS(0b100, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("0x0"))); /* 0x0 */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_sibs(stream) && parse_regi(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("{regi} * {2 ^ sibs}", eval_string_207_field_0, eval_string_207_field_1))); /* {regi} * {2 ^ sibs} */
    }
    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_sibsi(ByteStream* stream) {
	ParseRet res= parse_sibsi_(stream);
	if (!res.success) return res.success;
	aval_sibsi= res.aval;
	return res.success;
}

AVAL get_aval_sibsi() {
	return aval_sibsi;
}

ParseRet parse_SIB_INT_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_sibsi(stream) && EXPECT_BITS(0b101, stream)){
                    size_t pos_save_0= stream->pointer;

if (data_Mod._value == 0b00) {
                            size_t pos_save_0= stream->pointer;

                if (parse_disp32(stream)){
                    return PARSE_SUCC(to_aval(evaluate_string("{sibsi} + {disp32}", eval_string_208_field_0, eval_string_208_field_1))); /* {sibsi} + {disp32} */
                }
                stream->pointer= pos_save_0;


            }

            stream->pointer= pos_save_0;

            size_t pos_save_1= stream->pointer;

if (data_Mod._value == 0b01) {
                            size_t pos_save_0= stream->pointer;

                if (parse_disp8(stream)){
                    return PARSE_SUCC(to_aval(evaluate_string("{sibsi} + {disp8} + [EBP]", eval_string_209_field_0, eval_string_209_field_1))); /* {sibsi} + {disp8} + [EBP] */
                }
                stream->pointer= pos_save_0;


            }

            stream->pointer= pos_save_1;

            size_t pos_save_2= stream->pointer;

if (data_Mod._value == 0b10) {
                            size_t pos_save_0= stream->pointer;

                if (parse_disp32(stream)){
                    return PARSE_SUCC(to_aval(evaluate_string("{sibsi} + {disp32} + [EBP]", eval_string_210_field_0, eval_string_210_field_1))); /* {sibsi} + {disp32} + [EBP] */
                }
                stream->pointer= pos_save_0;


            }

            stream->pointer= pos_save_2;


    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_sibsi(stream) && parse_regb(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("{regb} + {sibsi}", eval_string_211_field_0, eval_string_211_field_1))); /* {regb} + {sibsi} */
    }
    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_SIB_INT(ByteStream* stream) {
	ParseRet res= parse_SIB_INT_(stream);
	if (!res.success) return res.success;
	aval_SIB_INT= res.aval;
	return res.success;
}

AVAL get_aval_SIB_INT() {
	return aval_SIB_INT;
}

ParseRet parse_SIB_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_SIB_INT(stream)){
        return PARSE_SUCC(get_aval_SIB_INT());
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_SIB(ByteStream* stream) {
	ParseRet res= parse_SIB_(stream);
	if (!res.success) return res.success;
	aval_SIB= res.aval;
	return res.success;
}

AVAL get_aval_SIB() {
	return aval_SIB;
}

ParseRet parse_rm_INT_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (data_Mod._value != 0b11) {
            size_t pos_save_0= stream->pointer;

        if (EXPECT_BITS(0b100, stream) && parse_SIB(stream)){
            return PARSE_SUCC(get_aval_SIB());
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

if (data_Mod._value == 0b00) {
            size_t pos_save_0= stream->pointer;

        if (EXPECT_BITS(0b101, stream) && parse_disp32(stream)){
                            size_t pos_save_0= stream->pointer;

if (get_flag_mode() == FLAG_MODE_VALUE_64bit) {
                return PARSE_SUCC(to_aval(evaluate_string("rip + {disp32}", eval_string_212_field_0))); /* rip + {disp32} */
                }

                stream->pointer= pos_save_0;

                size_t pos_save_1= stream->pointer;

                if (parse_disp32(stream)) return PARSE_SUCC(data_to_aval(data_disp32._value));
                stream->pointer= pos_save_1;


        }
        stream->pointer= pos_save_0;

        size_t pos_save_1= stream->pointer;

        if (parse_regb(stream)){
            return PARSE_SUCC(get_aval_regb());
        }
        stream->pointer= pos_save_1;


    }

    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

if (data_Mod._value == 0b01) {
            size_t pos_save_0= stream->pointer;

        if (parse_regb(stream) && parse_disp8(stream)){
            return PARSE_SUCC(to_aval(evaluate_string("{regb} + {disp8}", eval_string_213_field_0, eval_string_213_field_1))); /* {regb} + {disp8} */
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

if (data_Mod._value == 0b10) {
            size_t pos_save_0= stream->pointer;

        if (parse_regb(stream) && parse_disp32(stream)){
            return PARSE_SUCC(to_aval(evaluate_string("{regb} + {disp32}", eval_string_214_field_0, eval_string_214_field_1))); /* {regb} + {disp32} */
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_3;

    size_t pos_save_4= stream->pointer;

if (data_Mod._value == 0b11) {
            size_t pos_save_0= stream->pointer;

        if (parse_regop(stream)){
            return PARSE_SUCC(get_aval_regop());
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_4;

	return PARSE_FAIL; 
}

bool parse_rm_INT(ByteStream* stream) {
	ParseRet res= parse_rm_INT_(stream);
	if (!res.success) return res.success;
	aval_rm_INT= res.aval;
	return res.success;
}

AVAL get_aval_rm_INT() {
	return aval_rm_INT;
}

ParseRet parse_rm_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (data_Mod._value == 0b11) {
            size_t pos_save_0= stream->pointer;

        if (parse_rm_INT(stream)){
            return PARSE_SUCC(get_aval_rm_INT());
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_rm_INT(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("[{rm_INT}]", eval_string_215_field_0))); /* [{rm_INT}] */
    }
    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_rm(ByteStream* stream) {
	ParseRet res= parse_rm_(stream);
	if (!res.success) return res.success;
	aval_rm= res.aval;
	return res.success;
}

AVAL get_aval_rm() {
	return aval_rm;
}

ParseRet parse_rm_ptr_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (data_Mod._value == 0b11) {
    return PARSE_SUCC(get_aval_rm());
    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

if (0b1) {
    return PARSE_SUCC(to_aval(evaluate_string("{addr_ptr} {rm}", eval_string_216_field_0, eval_string_216_field_1))); /* {addr_ptr} {rm} */
    }

    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_rm_ptr(ByteStream* stream) {
	ParseRet res= parse_rm_ptr_(stream);
	if (!res.success) return res.success;
	aval_rm_ptr= res.aval;
	return res.success;
}

AVAL get_aval_rm_ptr() {
	parse_rm_ptr(&top_stream);
	return aval_rm_ptr;
}

ParseRet parse_ModRM_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_Mod(stream) && parse_regr(stream) && parse_rm(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("{regr}, {rm}", eval_string_217_field_0, eval_string_217_field_1))); /* {regr}, {rm} */
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_ModRM(ByteStream* stream) {
	ParseRet res= parse_ModRM_(stream);
	if (!res.success) return res.success;
	aval_ModRM= res.aval;
	return res.success;
}

AVAL get_aval_ModRM() {
	return aval_ModRM;
}

void calculate_flag_opmode() {
    if (data_ow._value){
        flag_opmode= FLAG_OPMODE_VALUE_8bit; return;
    }
    if (data_REX.w){
        flag_opmode= FLAG_OPMODE_VALUE_64bit; return;
    }
    if (aval_lp3.parsed_successfully){
        flag_opmode= FLAG_OPMODE_VALUE_16bit; return;
    }
            flag_opmode= var_default_opmode; return;
}
ParseRet parse_imm_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_8bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm8(stream)){
            return PARSE_SUCC(data_to_aval(data_imm8._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_16bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm16(stream)){
            return PARSE_SUCC(data_to_aval(data_imm16._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_32bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm32(stream)){
            return PARSE_SUCC(data_to_aval(data_imm32._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_64bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm64(stream)){
            return PARSE_SUCC(data_to_aval(data_imm64._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_3;

	return PARSE_FAIL; 
}

bool parse_imm(ByteStream* stream) {
	ParseRet res= parse_imm_(stream);
	if (!res.success) return res.success;
	aval_imm= res.aval;
	return res.success;
}

AVAL get_aval_imm() {
	return aval_imm;
}

ParseRet parse_immM32_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_8bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm8(stream)){
            return PARSE_SUCC(data_to_aval(data_imm8._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_16bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm16(stream)){
            return PARSE_SUCC(data_to_aval(data_imm16._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_32bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm32(stream)){
            return PARSE_SUCC(data_to_aval(data_imm32._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

if (get_flag_opmode() == FLAG_OPMODE_VALUE_64bit) {
            size_t pos_save_0= stream->pointer;

        if (parse_imm32(stream)){
            return PARSE_SUCC(data_to_aval(data_imm32._value));
        }
        stream->pointer= pos_save_0;


    }

    stream->pointer= pos_save_3;

	return PARSE_FAIL; 
}

bool parse_immM32(ByteStream* stream) {
	ParseRet res= parse_immM32_(stream);
	if (!res.success) return res.success;
	aval_immM32= res.aval;
	return res.success;
}

AVAL get_aval_immM32() {
	return aval_immM32;
}

DATA_MS parse_ms_(ByteStream* stream) {
	uint64_t res= read_bits(stream, 1);
	return (DATA_MS){._value= res, .parsed= true};
}

bool parse_ms(ByteStream* stream) {
	const DATA_MS res= parse_ms_(stream);
	if (!res.parsed) return false;
	data_ms= res;
	return true;
}

ParseRet parse_ModRMS_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (data_ms._value == 0b0) {
    return PARSE_SUCC(to_aval(evaluate_string("{rm_ptr}, {regr}", eval_string_218_field_0, eval_string_218_field_1))); /* {rm_ptr}, {regr} */
    }

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

if (data_ms._value == 0b1) {
    return PARSE_SUCC(to_aval(evaluate_string("{regr}, {rm}", eval_string_219_field_0, eval_string_219_field_1))); /* {regr}, {rm} */
    }

    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_ModRMS(ByteStream* stream) {
	ParseRet res= parse_ModRMS_(stream);
	if (!res.success) return res.success;
	aval_ModRMS= res.aval;
	return res.success;
}

AVAL get_aval_ModRMS() {
	parse_ModRMS(&top_stream);
	return aval_ModRMS;
}

ParseRet parse_MOV_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b1000, stream) && EXPECT_BITS(0b10, stream) && parse_ms(stream) && parse_ow(stream)&& ((data_ow._value= ~data_ow._value) || 1) && parse_ModRM(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {ModRMS}", eval_string_220_field_0))); /* MOV {ModRMS} */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b1100, stream) && EXPECT_BITS(0b011, stream) && parse_ow(stream)&& ((data_ow._value= ~data_ow._value) || 1) && parse_Mod(stream) && EXPECT_BITS(0b000, stream) && parse_rm(stream) && parse_immM32(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {rm_ptr}, {immM32}", eval_string_221_field_0, eval_string_221_field_1))); /* MOV {rm_ptr}, {immM32} */
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b1011, stream) && parse_ow(stream)&& ((data_ow._value= ~data_ow._value) || 1) && parse_regO(stream) && parse_imm(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("MOV {regT}, {imm}", eval_string_222_field_0, eval_string_222_field_1))); /* MOV {regT}, {imm} */
    }
    stream->pointer= pos_save_2;

	return PARSE_FAIL; 
}

bool parse_MOV(ByteStream* stream) {
	ParseRet res= parse_MOV_(stream);
	if (!res.success) return res.success;
	aval_MOV= res.aval;
	return res.success;
}

AVAL get_aval_MOV() {
	return aval_MOV;
}

ParseRet parse_regA_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

if (0b1) {
    ByteStream stream= stream_create();
    stream_add(&stream, 0b0000, 4);
    ParseRet res= parse_reg_(&stream);
    stream_destroy(&stream);
    return res;
    }

    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_regA(ByteStream* stream) {
	ParseRet res= parse_regA_(stream);
	if (!res.success) return res.success;
	aval_regA= res.aval;
	return res.success;
}

AVAL get_aval_regA() {
	parse_regA(&top_stream);
	return aval_regA;
}

ParseRet parse_ADC_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b0000, stream) && EXPECT_BITS(0b111, stream) && parse_ow(stream) && parse_immM32(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("ADC RAX, {immM32}", eval_string_223_field_0))); /* ADC RAX, {immM32} */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b1000, stream) && EXPECT_BITS(0b000, stream) && parse_ow(stream) && parse_immM32(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {reg}, {immM32}", eval_string_224_field_0, eval_string_224_field_1))); /* ADC {reg}, {immM32} */
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b1000, stream) && EXPECT_BITS(0b0011, stream) && parse_Mod(stream) && EXPECT_BITS(0b010, stream) && parse_regO(stream) && parse_imm8(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {reg}, {imm8}", eval_string_225_field_0, eval_string_225_field_1))); /* ADC {reg}, {imm8} */
    }
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (EXPECT_BITS(0b0001, stream) && EXPECT_BITS(0b00, stream) && parse_ms(stream) && parse_ow(stream) && parse_ModRM(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("ADC {ModRMS}", eval_string_226_field_0))); /* ADC {ModRMS} */
    }
    stream->pointer= pos_save_3;

	return PARSE_FAIL; 
}

bool parse_ADC(ByteStream* stream) {
	ParseRet res= parse_ADC_(stream);
	if (!res.success) return res.success;
	aval_ADC= res.aval;
	return res.success;
}

AVAL get_aval_ADC() {
	return aval_ADC;
}

ParseRet parse_PUSH_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    FLAG_OPMODE save_default_opmode_0= var_default_opmode;
    var_default_opmode= FLAG_OPMODE_VALUE_64bit;
    ParseRet res_0= parse_with_0(stream);
    var_default_opmode= save_default_opmode_0;
    if (res_0.success) return res_0;

    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b0110, stream) && EXPECT_BITS(0b10, stream) && parse_ow(stream) && EXPECT_BITS(0b0, stream) && parse_immM32(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("PUSH {immM32}", eval_string_229_field_0))); /* PUSH {immM32} */
    }
    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_PUSH(ByteStream* stream) {
	ParseRet res= parse_PUSH_(stream);
	if (!res.success) return res.success;
	aval_PUSH= res.aval;
	return res.success;
}

AVAL get_aval_PUSH() {
	return aval_PUSH;
}

ParseRet parse_CMOV_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b0100, stream) && parse_cc(stream) && parse_ModRM(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("CMOV{cc} {ModRM}", eval_string_230_field_0, eval_string_230_field_1))); /* CMOV{cc} {ModRM} */
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_CMOV(ByteStream* stream) {
	ParseRet res= parse_CMOV_(stream);
	if (!res.success) return res.success;
	aval_CMOV= res.aval;
	return res.success;
}

AVAL get_aval_CMOV() {
	return aval_CMOV;
}

ParseRet parse_ModRM_5_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_Mod(stream) && EXPECT_BITS(0b101, stream) && parse_rm(stream)){
        return PARSE_SUCC(get_aval_rm());
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_ModRM_5(ByteStream* stream) {
	ParseRet res= parse_ModRM_5_(stream);
	if (!res.success) return res.success;
	aval_ModRM_5= res.aval;
	return res.success;
}

AVAL get_aval_ModRM_5() {
	return aval_ModRM_5;
}

ParseRet parse_ModRM_2_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (parse_Mod(stream) && EXPECT_BITS(0b010, stream) && parse_rm(stream)){
        return PARSE_SUCC(get_aval_rm());
    }
    stream->pointer= pos_save_0;

	return PARSE_FAIL; 
}

bool parse_ModRM_2(ByteStream* stream) {
	ParseRet res= parse_ModRM_2_(stream);
	if (!res.success) return res.success;
	aval_ModRM_2= res.aval;
	return res.success;
}

AVAL get_aval_ModRM_2() {
	return aval_ModRM_2;
}

ParseRet parse_SUB_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BITS(0b0010, stream) && EXPECT_BITS(0b110, stream) && parse_ow(stream)&& ((data_ow._value= ~data_ow._value) || 1) && parse_immM32(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("SUB {regA}, {immM32}", eval_string_231_field_0, eval_string_231_field_1))); /* SUB {regA}, {immM32} */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (EXPECT_BITS(0b1000, stream) && EXPECT_BITS(0b000, stream) && parse_ow(stream)&& ((data_ow._value= ~data_ow._value) || 1) && parse_ModRM_5(stream) && parse_immM32(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("SUB {ModRM_5}, {immM32}", eval_string_232_field_0, eval_string_232_field_1))); /* SUB {ModRM_5}, {immM32} */
    }
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (EXPECT_BITS(0b1000, stream) && EXPECT_BITS(0b0011, stream) && parse_ModRM_5(stream) && parse_imm8(stream)){
        return PARSE_SUCC(to_aval(evaluate_string("SUB {ModRM_5}, {imm8}", eval_string_233_field_0, eval_string_233_field_1))); /* SUB {ModRM_5}, {imm8} */
    }
    stream->pointer= pos_save_2;

	return PARSE_FAIL; 
}

bool parse_SUB(ByteStream* stream) {
	ParseRet res= parse_SUB_(stream);
	if (!res.success) return res.success;
	aval_SUB= res.aval;
	return res.success;
}

AVAL get_aval_SUB() {
	return aval_SUB;
}

ParseRet parse_op2_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0x1e, stream) && EXPECT_BYTE(0xfa, stream)){
        return PARSE_SUCC(to_aval(evaluate_string("ENDBR64"))); /* ENDBR64 */
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_CMOV(stream)) return PARSE_SUCC(get_aval_CMOV());
    stream->pointer= pos_save_1;

	return PARSE_FAIL; 
}

bool parse_op2(ByteStream* stream) {
	ParseRet res= parse_op2_(stream);
	if (!res.success) return res.success;
	aval_op2= res.aval;
	return res.success;
}

AVAL get_aval_op2() {
	return aval_op2;
}

ParseRet parse_op_(ByteStream* stream) {
    size_t pos_save_0= stream->pointer;

    if (EXPECT_BYTE(0xf, stream) && parse_op2(stream)){
        return PARSE_SUCC(get_aval_op2());
    }
    stream->pointer= pos_save_0;

    size_t pos_save_1= stream->pointer;

    if (parse_MOV(stream)) return PARSE_SUCC(get_aval_MOV());
    stream->pointer= pos_save_1;

    size_t pos_save_2= stream->pointer;

    if (parse_PUSH(stream)) return PARSE_SUCC(get_aval_PUSH());
    stream->pointer= pos_save_2;

    size_t pos_save_3= stream->pointer;

    if (parse_SUB(stream)) return PARSE_SUCC(get_aval_SUB());
    stream->pointer= pos_save_3;

	return PARSE_FAIL; 
}

bool parse_op(ByteStream* stream) {
	ParseRet res= parse_op_(stream);
	if (!res.success) return res.success;
	aval_op= res.aval;
	return res.success;
}

AVAL get_aval_op() {
	return aval_op;
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
	aval_regO.choices= vector_create();
	aval_regM.choices= vector_create();
	aval_cc.choices= vector_create();
	aval_regi.choices= vector_create();
	aval_regb.choices= vector_create();
	aval_regop.choices= vector_create();
	aval_regr.choices= vector_create();
	aval_addr_ptr.choices= vector_create();
	aval_sibsi.choices= vector_create();
	aval_SIB_INT.choices= vector_create();
	aval_SIB.choices= vector_create();
	aval_rm_INT.choices= vector_create();
	aval_rm.choices= vector_create();
	aval_rm_ptr.choices= vector_create();
	aval_ModRM.choices= vector_create();
	aval_imm.choices= vector_create();
	aval_immM32.choices= vector_create();
	aval_ModRMS.choices= vector_create();
	aval_MOV.choices= vector_create();
	aval_regA.choices= vector_create();
	aval_ADC.choices= vector_create();
	aval_PUSH.choices= vector_create();
	aval_CMOV.choices= vector_create();
	aval_ModRM_5.choices= vector_create();
	aval_ModRM_2.choices= vector_create();
	aval_SUB.choices= vector_create();
	aval_op2.choices= vector_create();
	aval_op.choices= vector_create();
	return 0;
}

