

#ifndef _A64_INSTRUCTIONS_H_
#define _A64_INSTRUCTIONS_H_

#include "types.h"

typedef u32_t a64_t;

typedef enum a64_reg {
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
	R31 = 31,
	RSP=0x1F
} a64_reg_t;

#define XZR (R31)
#define SP (R31)

typedef enum a64_sz {
	SZX = 0b11,
	SZW = 0b10,
	SZH = 0b01,
	SZB = 0b00
} a64_sz_t;

typedef enum a64_cond {
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
	NV	= 0b1111,
	B
} a64_cond_t;

#define LO (CC)

typedef enum a64_addressing_mode {
	IMM,
	PRE = 0b11,
	POST = 0b01
} a64_addressing_mode_t;

typedef enum ldr_str {
	LOAD,
	STORE
} ldr_str_t;

//a64_t a64_branch(i32_t imm);

//https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions/B-cond--Branch-conditionally-?lang=en
a64_t a64_b(a64_cond_t cond, i32_t imm);

//https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions/BR--Branch-to-register-?lang=en
a64_t a64_br(a64_reg_t r);

a64_t a64_cbzw(a64_reg_t r, i32_t imm19);
a64_t a64_cbz(a64_reg_t r, i32_t imm19);
a64_t a64_cbnzw(a64_reg_t r, i32_t imm19);
a64_t a64_cbnz(a64_reg_t r, i32_t imm19);
//https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions/BL--Branch-with-link-?lang=en
a64_t a64_bl(a64_t imm);

a64_t a64_bli(a64_t imm);

//https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions/BLR--Branch-with-link-to-register-?lang=en
a64_t a64_blr(a64_reg_t r);

//https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions/CMP--immediate---Compare--immediate---an-alias-of-SUBS--immediate--?lang=en
a64_t a64_cmpiw(a64_reg_t reg, i32_t imm);

//https://developer.arm.com/documentation/ddi0602/2024-09/Base-Instructions/CMP--immediate---Compare--immediate---an-alias-of-SUBS--immediate--?lang=en
a64_t a64_cmpi(a64_reg_t reg, i32_t imm);

a64_t a64_mov(a64_reg_t dst, a64_reg_t src);
a64_t a64_movw(a64_reg_t dst, a64_reg_t src);
a64_t a64_movi(a64_reg_t dst, u64_t imm);
a64_t a64_moviw(a64_reg_t dst, u64_t imm);

a64_t a64_addi(a64_reg_t dst, a64_reg_t src, i32_t imm);
a64_t a64_subi(a64_reg_t dst, a64_reg_t src, i32_t imm);



a64_t a64_adr(a64_reg_t dst, i32_t imm21);

a64_t a64_adc(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_adcw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_adcs(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_adcsw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_sbc(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_sbcw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_sbcs(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_sbcsw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_add(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_addw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_adds(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_addsw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_sub(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_subw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_subs(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_subsw(a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);

a64_t a64_cmp(a64_reg_t r1, a64_reg_t r2);
a64_t a64_cmpw(a64_reg_t r1, a64_reg_t r2);



a64_t a64_load_store(
	ldr_str_t		ldst,
	a64_addressing_mode_t	mode,
	a64_sz_t		sz,
	bool			sign_extend,
	a64_reg_t		dst,
	a64_reg_t		addr,
	i64_t imm);

static inline a64_t a64_ret(void) { return 0xd65f03c0; }
static inline a64_t a64_nop(void) { return 0xd503201f; }

a64_t a64_brk(u32_t imm16);

typedef enum a64_simd_sz {
	SIMD_B = 0,
	SIMD_H = 1,
	SIMD_S = 2,
	SIMD_D = 3
} a64_simd_sz_t;

typedef enum a64_simd_q {
	SIMD_HALF = 0,
	SIMD_FULL = 1
} a64_simd_q_t;

typedef enum a64_simd_ld1_mode {
	LD1_NONE,
	LD1_IMM,
	LD1_REG
} a64_simd_ld1_mode_t;

//a64_t a64_simd_ld1(a64_simd_sz_t size, a64_simd_q_t q, u32_t cnt, a64_reg_t dst, a64_reg_t addr);
a64_t a64_simd_ld1(a64_simd_ld1_mode_t mode, a64_simd_sz_t size, a64_simd_q_t q, u32_t cnt, a64_reg_t dst, a64_reg_t addr, ...);
a64_t a64_simd_and(a64_simd_q_t q, a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_simd_not(a64_simd_q_t q, a64_reg_t dst, a64_reg_t r);
static inline a64_t a64_simd_mvn(a64_simd_q_t q, a64_reg_t r) { return a64_simd_not(q,r,r); }
a64_t a64_simd_cmeq(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_simd_cmhi(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);
a64_t a64_simd_cmhs(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t s1, a64_reg_t s2);

a64_t a64_simd_umaxv(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t src);
a64_t a64_simd_uminv(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t src);
a64_t a64_simd_umov(a64_simd_sz_t size, u32_t idx, a64_reg_t dst, a64_reg_t src);

a64_t a64_simd_movi(a64_simd_q_t q, u32_t op, u32_t cmode, u8_t imm, a64_reg_t dst);
a64_t a64_simd_movib(a64_simd_q_t q, a64_reg_t dst, u8_t imm8);
a64_t a64_simd_umaxp(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t r1, a64_reg_t r2);
a64_t a64_simd_uminp(a64_simd_sz_t size, a64_simd_q_t q, a64_reg_t dst, a64_reg_t r1, a64_reg_t r2);
#endif /* _A64_INSTRUCTIONS_H_ */