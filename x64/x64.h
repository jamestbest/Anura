#include "default.h"

typedef struct DATA_OW {
	uint8_t parsed: 1;
	uint8_t _value: 1;
} DATA_OW;
DATA_OW data_ow;
bool parse_ow();

typedef struct DATA_REGX {
	uint8_t parsed: 1;
	uint8_t _value: 3;
} DATA_REGX;
DATA_REGX data_regx;
bool parse_regx();

typedef struct DATA_SIBS {
	uint8_t parsed: 1;
	uint8_t _value: 2;
} DATA_SIBS;
DATA_SIBS data_sibs;
bool parse_sibs();

typedef struct DATA_IMM8 {
	uint8_t parsed: 1;
	uint8_t _value: 8;
} DATA_IMM8;
DATA_IMM8 data_imm8;
bool parse_imm8();

typedef struct DATA_IMM16 {
	uint8_t parsed: 1;
	uint16_t _value: 16;
} DATA_IMM16;
DATA_IMM16 data_imm16;
bool parse_imm16();

typedef struct DATA_IMM32 {
	uint8_t parsed: 1;
	uint32_t _value: 32;
} DATA_IMM32;
DATA_IMM32 data_imm32;
bool parse_imm32();

typedef struct DATA_IMM64 {
	uint8_t parsed: 1;
	uint64_t _value: 64;
} DATA_IMM64;
DATA_IMM64 data_imm64;
bool parse_imm64();

typedef struct DATA_DISP8 {
	uint8_t parsed: 1;
	uint8_t _value: 8;
} DATA_DISP8;
DATA_DISP8 data_disp8;
bool parse_disp8();

typedef struct DATA_DISP16 {
	uint8_t parsed: 1;
	uint16_t _value: 16;
} DATA_DISP16;
DATA_DISP16 data_disp16;
bool parse_disp16();

typedef struct DATA_DISP32 {
	uint8_t parsed: 1;
	uint32_t _value: 32;
} DATA_DISP32;
DATA_DISP32 data_disp32;
bool parse_disp32();

typedef struct DATA_REX {
	uint8_t parsed: 1;
	uint8_t w: 1;
	uint8_t r: 1;
	uint8_t x: 1;
	uint8_t b: 1;
} DATA_REX;
DATA_REX data_REX;
bool parse_REX();

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
DATA_VEX data_VEX;
bool parse_VEX();

AVAL aval_prefix= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_lp1= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_lp2= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_lp3= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_lp4= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_lprefix= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_reg= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_regT= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_cc= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

typedef struct DATA_REGI {
	uint8_t parsed: 1;
	uint8_t _value: 3;
} DATA_REGI;
DATA_REGI data_regi;
bool parse_regi();

typedef struct DATA_REGB {
	uint8_t parsed: 1;
	uint8_t _value: 3;
} DATA_REGB;
DATA_REGB data_regb;
bool parse_regb();

typedef struct DATA_REGR {
	uint8_t parsed: 1;
	uint8_t _value: 3;
} DATA_REGR;
DATA_REGR data_regr;
bool parse_regr();

typedef struct DATA_MOD {
	uint8_t parsed: 1;
	uint8_t _value: 2;
} DATA_MOD;
DATA_MOD data_Mod;
bool parse_Mod();

typedef struct DATA_RM {
	uint8_t parsed: 1;
	uint8_t _value: 3;
} DATA_RM;
DATA_RM data_RM;
bool parse_RM();

AVAL aval_sibsi= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_SIB= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_ModRM= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_imm= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_immM32= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_MOV= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_ADC= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_CMOV= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_op2= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_op= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_reg_a= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_reg_b= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

AVAL aval_name= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};

