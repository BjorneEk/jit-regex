

#ifndef _AARCH64_INSTRUCTIONS_H_
#define _AARCH64_INSTRUCTIONS_H_

#include "../util/types.h"


typedef u32_t aarch64_t;

typedef enum aarch64_reg {
	R0	= 0,
	R1	= 1,
	R2	= 2,
	R3	= 3,
	R4	= 4,
	R5	= 5,
	R6	= 6,
	R7	= 7,
	R8	= 8,
	R9	= 9,
	R10	= 10,
	R11	= 11,
	R12	= 12,
	R13	= 13,
	R14	= 14,
	R15	= 15,
	R16	= 16,
	R17	= 17,
	R18	= 18,
	R19	= 19,
	R20	= 20,
	R21	= 21,
	R22	= 22,
	R23	= 23,
	R24	= 24,
	R25	= 25,
	R26	= 26,
	R27	= 27,
	R28	= 28,
	R29	= 29,
	R30	= 30,
	XZR = 31,
} aarch64_reg_t;

typedef enum aarch64_cond {
	EQ	= 0b0000,
	NE	= 0b0001,
	CS	= 0b0010,
	CC	= 0b0011,
	MI	= 0b0100,
	PL	= 0b0101,
	VS	= 0b0110,
	VC	= 0b0111,
	HI	= 0b1000,
	LS	= 0b1001,
	GE	= 0b1010,
	LT	= 0b1011,
	GT	= 0b1100,
	LE	= 0b1101,
	AL	= 0b1110,
	NV	= 0b1111
} aarch64_cond_t;

typedef enum aarch64_load {
	LDRX	= 0b111110010100000000000000000000, //ldr	x0, [x0] 8-byte
	LDRW	= 0b101110010100000000000000000000, //ldr	w0, [x0] 4-byte
	LDRH	= 0b011110010100000000000000000000, //ldrh	w0, [x0] 2-byte
	LDRB	= 0b001110010100000000000000000000, //ldrb	w0, [x0] 1-byte
	LDRSH	= 0b011110011100000000000000000000, //ldrsh	w0, [x0] signed 2-byte
	LDRSB	= 0b001110011100000000000000000000, //ldrsb	w0, [x0] signed 1-byte
} aarch64_load_t;

typedef enum aarch64_store {
	STRX	= LDRX ^ (1 << 20), //ldr	x0, [x0] 8-byte
	STRW	= LDRW ^ (1 << 20), //ldr	w0, [x0] 4-byte
	STRH	= LDRH ^ (1 << 20), //ldrh	w0, [x0] 2-byte
	STRB	= LDRB ^ (1 << 20), //ldrb	w0, [x0] 1-byte
} aarch64_store_t;

aarch64_t aarch64_branch(i32_t imm);

aarch64_t aarch64_cond_branch(aarch64_cond_t cond, i32_t imm19);

aarch64_t aarch64_cmpw_imm(aarch64_reg_t reg, i32_t imm);

aarch64_t aarch64_cmpx_imm(aarch64_reg_t reg, i32_t imm);

aarch64_t aarch64_ldr(aarch64_load_t ins, aarch64_reg_t dst, aarch64_reg_t addr_reg);

aarch64_t aarch64_str(aarch64_store_t ins, aarch64_reg_t dst, aarch64_reg_t addr_reg);

aarch64_t aarch64_mov(aarch64_reg_t dst, aarch64_reg_t src);

aarch64_t aarch64_add_imm(aarch64_reg_t dst, aarch64_reg_t src, i32_t imm);

aarch64_t aarch64_sub_imm(aarch64_reg_t dst, aarch64_reg_t src, i32_t imm);
#endif /* _AARCH64_INSTRUCTIONS_H_ */