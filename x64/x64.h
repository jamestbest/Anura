#include "default.h"

void eval_string_207_field_0(Buffer* buff);
void eval_string_207_field_1(Buffer* buff);

void eval_string_208_field_0(Buffer* buff);
void eval_string_208_field_1(Buffer* buff);

void eval_string_209_field_0(Buffer* buff);
void eval_string_209_field_1(Buffer* buff);

void eval_string_210_field_0(Buffer* buff);
void eval_string_210_field_1(Buffer* buff);

void eval_string_211_field_0(Buffer* buff);
void eval_string_211_field_1(Buffer* buff);

void eval_string_212_field_0(Buffer* buff);

void eval_string_213_field_0(Buffer* buff);
void eval_string_213_field_1(Buffer* buff);

void eval_string_214_field_0(Buffer* buff);
void eval_string_214_field_1(Buffer* buff);

void eval_string_215_field_0(Buffer* buff);

void eval_string_216_field_0(Buffer* buff);
void eval_string_216_field_1(Buffer* buff);

void eval_string_217_field_0(Buffer* buff);
void eval_string_217_field_1(Buffer* buff);

void eval_string_218_field_0(Buffer* buff);
void eval_string_218_field_1(Buffer* buff);

void eval_string_219_field_0(Buffer* buff);
void eval_string_219_field_1(Buffer* buff);

void eval_string_220_field_0(Buffer* buff);

void eval_string_221_field_0(Buffer* buff);
void eval_string_221_field_1(Buffer* buff);

void eval_string_222_field_0(Buffer* buff);
void eval_string_222_field_1(Buffer* buff);

void eval_string_223_field_0(Buffer* buff);

void eval_string_224_field_0(Buffer* buff);
void eval_string_224_field_1(Buffer* buff);

void eval_string_225_field_0(Buffer* buff);
void eval_string_225_field_1(Buffer* buff);

void eval_string_226_field_0(Buffer* buff);

void eval_string_227_field_0(Buffer* buff);

void eval_string_228_field_0(Buffer* buff);

void eval_string_229_field_0(Buffer* buff);

void eval_string_230_field_0(Buffer* buff);
void eval_string_230_field_1(Buffer* buff);

void eval_string_231_field_0(Buffer* buff);
void eval_string_231_field_1(Buffer* buff);

void eval_string_232_field_0(Buffer* buff);
void eval_string_232_field_1(Buffer* buff);

void eval_string_233_field_0(Buffer* buff);
void eval_string_233_field_1(Buffer* buff);

ParseRet parse_with_0(ByteStream* stream);

typedef enum FLAG_MODE{
	FLAG_MODE_VALUE_64bit,
	FLAG_MODE_VALUE_32bit,
	FLAG_MODE_VALUE_16bit 
} FLAG_MODE;
FLAG_MODE flag_mode= FLAG_MODE_VALUE_64bit;
FLAG_MODE get_flag_mode();
bool flag_calculated_mode= false;

typedef enum FLAG_OPMODE{
	FLAG_OPMODE_VALUE_64bit,
	FLAG_OPMODE_VALUE_32bit,
	FLAG_OPMODE_VALUE_16bit,
	FLAG_OPMODE_VALUE_8bit 
} FLAG_OPMODE;
FLAG_OPMODE flag_opmode= FLAG_OPMODE_VALUE_32bit;
FLAG_OPMODE get_flag_opmode();
bool flag_calculated_opmode= false;

typedef enum FLAG_ADDRMODE{
	FLAG_ADDRMODE_VALUE_64bit,
	FLAG_ADDRMODE_VALUE_32bit,
	FLAG_ADDRMODE_VALUE_16bit 
} FLAG_ADDRMODE;
FLAG_ADDRMODE flag_addrmode= FLAG_ADDRMODE_VALUE_64bit;
FLAG_ADDRMODE get_flag_addrmode();
bool flag_calculated_addrmode= false;

FLAG_OPMODE var_default_opmode= FLAG_OPMODE_VALUE_32bit;
typedef struct DATA_OW {
	uint8_t parsed: 1;
	uint8_t _value: 1;
} DATA_OW;
DATA_OW data_ow= {0};
bool parse_ow(ByteStream* stream);

typedef struct DATA_REGX {
	uint8_t parsed: 1;
	uint8_t _value: 3;
} DATA_REGX;
DATA_REGX data_regx= {0};
bool parse_regx(ByteStream* stream);

typedef struct DATA_SIBS {
	uint8_t parsed: 1;
	uint8_t _value: 2;
} DATA_SIBS;
DATA_SIBS data_sibs= {0};
bool parse_sibs(ByteStream* stream);

typedef struct DATA_IMM8 {
	uint8_t parsed: 1;
	uint8_t _value: 8;
} DATA_IMM8;
DATA_IMM8 data_imm8= {0};
bool parse_imm8(ByteStream* stream);

typedef struct DATA_IMM16 {
	uint8_t parsed: 1;
	uint16_t _value: 16;
} DATA_IMM16;
DATA_IMM16 data_imm16= {0};
bool parse_imm16(ByteStream* stream);

typedef struct DATA_IMM32 {
	uint8_t parsed: 1;
	uint32_t _value: 32;
} DATA_IMM32;
DATA_IMM32 data_imm32= {0};
bool parse_imm32(ByteStream* stream);

typedef struct DATA_IMM64 {
	uint8_t parsed: 1;
	uint64_t _value: 64;
} DATA_IMM64;
DATA_IMM64 data_imm64= {0};
bool parse_imm64(ByteStream* stream);

typedef struct DATA_DISP8 {
	uint8_t parsed: 1;
	uint8_t _value: 8;
} DATA_DISP8;
DATA_DISP8 data_disp8= {0};
bool parse_disp8(ByteStream* stream);

typedef struct DATA_DISP16 {
	uint8_t parsed: 1;
	uint16_t _value: 16;
} DATA_DISP16;
DATA_DISP16 data_disp16= {0};
bool parse_disp16(ByteStream* stream);

typedef struct DATA_DISP32 {
	uint8_t parsed: 1;
	uint32_t _value: 32;
} DATA_DISP32;
DATA_DISP32 data_disp32= {0};
bool parse_disp32(ByteStream* stream);

typedef struct DATA_REX {
	uint8_t parsed: 1;
	uint8_t w: 1;
	uint8_t r: 1;
	uint8_t x: 1;
	uint8_t b: 1;
} DATA_REX;
DATA_REX data_REX= {
	.w= 0,
};
bool parse_REX(ByteStream* stream);

typedef struct DATA_VEX {
	uint8_t parsed: 1;
	uint8_t R: 1;
	uint8_t X: 1;
	uint8_t B: 1;
	uint8_t m: 5;
	uint8_t W: 1;
	uint8_t v: 4;
	uint8_t L: 1;
	uint8_t pp: 2;
} DATA_VEX;
DATA_VEX data_VEX= {0};
bool parse_VEX(ByteStream* stream);

AVAL aval_prefix= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_prefix();
bool parse_prefix(ByteStream* stream);
ParseRet parse_prefix_(ByteStream* stream);

AVAL aval_lp1= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_lp1();
bool parse_lp1(ByteStream* stream);
ParseRet parse_lp1_(ByteStream* stream);

AVAL aval_lp2= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_lp2();
bool parse_lp2(ByteStream* stream);
ParseRet parse_lp2_(ByteStream* stream);

AVAL aval_lp3= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_lp3();
bool parse_lp3(ByteStream* stream);
ParseRet parse_lp3_(ByteStream* stream);

AVAL aval_lp4= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_lp4();
bool parse_lp4(ByteStream* stream);
ParseRet parse_lp4_(ByteStream* stream);

AVAL aval_lprefix= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_lprefix();
bool parse_lprefix(ByteStream* stream);
ParseRet parse_lprefix_(ByteStream* stream);

AVAL aval_reg= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_reg();
bool parse_reg(ByteStream* stream);
ParseRet parse_reg_(ByteStream* stream);

AVAL aval_regT= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regT();
bool parse_regT(ByteStream* stream);
ParseRet parse_regT_(ByteStream* stream);

AVAL aval_regO= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regO();
bool parse_regO(ByteStream* stream);
ParseRet parse_regO_(ByteStream* stream);

AVAL aval_regM= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regM();
bool parse_regM(ByteStream* stream);
ParseRet parse_regM_(ByteStream* stream);

int calculate_rule_right_0();

int calculate_rule_right_1();

AVAL aval_cc= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_cc();
bool parse_cc(ByteStream* stream);
ParseRet parse_cc_(ByteStream* stream);

AVAL aval_regi= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regi();
bool parse_regi(ByteStream* stream);
ParseRet parse_regi_(ByteStream* stream);

AVAL aval_regb= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regb();
bool parse_regb(ByteStream* stream);
ParseRet parse_regb_(ByteStream* stream);

AVAL aval_regop= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regop();
bool parse_regop(ByteStream* stream);
ParseRet parse_regop_(ByteStream* stream);

AVAL aval_regr= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_regr();
bool parse_regr(ByteStream* stream);
ParseRet parse_regr_(ByteStream* stream);

typedef struct DATA_MOD {
	uint8_t parsed: 1;
	uint8_t _value: 2;
} DATA_MOD;
DATA_MOD data_Mod= {0};
bool parse_Mod(ByteStream* stream);

void calculate_flag_addrmode();AVAL aval_addr_ptr= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= true};
AVAL get_aval_addr_ptr();
bool parse_addr_ptr(ByteStream* stream);
ParseRet parse_addr_ptr_(ByteStream* stream);

AVAL aval_sibsi= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_sibsi();
bool parse_sibsi(ByteStream* stream);
ParseRet parse_sibsi_(ByteStream* stream);

AVAL aval_SIB_INT= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_SIB_INT();
bool parse_SIB_INT(ByteStream* stream);
ParseRet parse_SIB_INT_(ByteStream* stream);

AVAL aval_SIB= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_SIB();
bool parse_SIB(ByteStream* stream);
ParseRet parse_SIB_(ByteStream* stream);

AVAL aval_rm_INT= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_rm_INT();
bool parse_rm_INT(ByteStream* stream);
ParseRet parse_rm_INT_(ByteStream* stream);

AVAL aval_rm= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_rm();
bool parse_rm(ByteStream* stream);
ParseRet parse_rm_(ByteStream* stream);

AVAL aval_rm_ptr= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= true};
AVAL get_aval_rm_ptr();
bool parse_rm_ptr(ByteStream* stream);
ParseRet parse_rm_ptr_(ByteStream* stream);

AVAL aval_ModRM= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_ModRM();
bool parse_ModRM(ByteStream* stream);
ParseRet parse_ModRM_(ByteStream* stream);

void calculate_flag_opmode();AVAL aval_imm= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_imm();
bool parse_imm(ByteStream* stream);
ParseRet parse_imm_(ByteStream* stream);

AVAL aval_immM32= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_immM32();
bool parse_immM32(ByteStream* stream);
ParseRet parse_immM32_(ByteStream* stream);

typedef struct DATA_MS {
	uint8_t parsed: 1;
	uint8_t _value: 1;
} DATA_MS;
DATA_MS data_ms= {0};
bool parse_ms(ByteStream* stream);

AVAL aval_ModRMS= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= true};
AVAL get_aval_ModRMS();
bool parse_ModRMS(ByteStream* stream);
ParseRet parse_ModRMS_(ByteStream* stream);

AVAL aval_MOV= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_MOV();
bool parse_MOV(ByteStream* stream);
ParseRet parse_MOV_(ByteStream* stream);

AVAL aval_regA= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= true};
AVAL get_aval_regA();
bool parse_regA(ByteStream* stream);
ParseRet parse_regA_(ByteStream* stream);

AVAL aval_ADC= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_ADC();
bool parse_ADC(ByteStream* stream);
ParseRet parse_ADC_(ByteStream* stream);

AVAL aval_PUSH= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_PUSH();
bool parse_PUSH(ByteStream* stream);
ParseRet parse_PUSH_(ByteStream* stream);

AVAL aval_CMOV= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_CMOV();
bool parse_CMOV(ByteStream* stream);
ParseRet parse_CMOV_(ByteStream* stream);

AVAL aval_ModRM_5= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_ModRM_5();
bool parse_ModRM_5(ByteStream* stream);
ParseRet parse_ModRM_5_(ByteStream* stream);

AVAL aval_ModRM_2= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_ModRM_2();
bool parse_ModRM_2(ByteStream* stream);
ParseRet parse_ModRM_2_(ByteStream* stream);

AVAL aval_SUB= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_SUB();
bool parse_SUB(ByteStream* stream);
ParseRet parse_SUB_(ByteStream* stream);

AVAL aval_op2= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_op2();
bool parse_op2(ByteStream* stream);
ParseRet parse_op2_(ByteStream* stream);

AVAL aval_op= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= false};
AVAL get_aval_op();
bool parse_op(ByteStream* stream);
ParseRet parse_op_(ByteStream* stream);

