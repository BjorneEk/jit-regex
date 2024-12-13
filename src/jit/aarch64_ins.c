
#include "../util/types.h"
#include "aarch64_ins.h"
#include <stdlib.h>
#include <stdio.h>

aarch64_t aarch64_branch(i32_t imm)
{
	const aarch64_t imask		= 0b00010100000000000000000000000000;
	const aarch64_t getbits_mask	= 0b00000011111111111111111111111111;

	return imask | (imm & getbits_mask);
}


aarch64_t aarch64_cond_branch(aarch64_cond_t cond, i32_t imm19)
{
	return (0b01010100 << 24) | (imm19 & 0b1111111111111111111) << 5 | (cond & 0xF);
}

static aarch64_t aarch64_imm(i32_t imm)
{
	const
	aarch64_t imm_mask = 0b00000000011111111111110000000000;
	if (imm > 4096 || imm < -4096) {
		fprintf(stderr, "%d: immidiate value out of range [-4096 4096]\n", imm);
		exit(-1);
	}
	return (imm << 10) & imm_mask;
}

static aarch64_t aarch64_reg(aarch64_reg_t reg, u32_t off)
{
	const aarch64_t r_mask	= 0b00000000000000000000000000011111;
	return (reg << off) & (r_mask << off);
}

aarch64_t aarch64_cmpw_imm(aarch64_reg_t reg, i32_t imm)
{
	const aarch64_t imask		= 0b01110001000000000000000010011111;

	return aarch64_reg(reg, 5) | aarch64_imm(imm) | imask;
}

aarch64_t aarch64_cmpx_imm(aarch64_reg_t reg, i32_t imm)
{
	return aarch64_cmpw_imm(reg, imm) | (1 << 31);
}


aarch64_t aarch64_ldr(aarch64_load_t ins, aarch64_reg_t dst, aarch64_reg_t addr_reg)
{
	return aarch64_reg(dst, 5) | aarch64_reg(dst, 0) | (ins << 2);
}

aarch64_t aarch64_str(aarch64_store_t ins, aarch64_reg_t dst, aarch64_reg_t addr_reg)
{
	return aarch64_reg(dst, 5) | aarch64_reg(dst, 0) | (ins << 2);
}

aarch64_t aarch64_mov(aarch64_reg_t dst, aarch64_reg_t src)
{
	const aarch64_t imask	= 0b10101010000000000000001111100000;

	return aarch64_reg(src, 17) | aarch64_reg(dst, 0) | imask;
}

aarch64_t aarch64_add_imm(aarch64_reg_t dst, aarch64_reg_t src, i32_t imm)
{
	const aarch64_t imask		= 0b10010001000000000000000000000000;
	return aarch64_reg(dst, 5) | aarch64_reg(dst, 0) | aarch64_imm(imm) | imask;
}

aarch64_t aarch64_sub_imm(aarch64_reg_t dst, aarch64_reg_t src, i32_t imm)
{
	const aarch64_t imask		= 0b11010001000000000000000000000000;
	return aarch64_reg(dst, 5) | aarch64_reg(dst, 0) | aarch64_imm(imm) | imask;
}