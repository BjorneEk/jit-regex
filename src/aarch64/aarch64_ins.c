
#include "../util/types.h"
#include "aarch64_ins.h"
#include <stdlib.h>
#include <stdio.h>




static aarch64_t aarch64_uncond_branch(i32_t imm)
{
	const aarch64_t imask		= 0b00010100000000000000000000000000;
	const aarch64_t getbits_mask	= 0b00000011111111111111111111111111;

	return imask | (imm & getbits_mask);
}


aarch64_t aarch64_branch(aarch64_cond_t cond, i32_t imm)
{
	if (cond == B)
		return aarch64_uncond_branch(imm);
	return (0b01010100 << 24) | (imm & 0b1111111111111111111) << 5 | (cond & 0xF);
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

aarch64_t aarch64_cmpw(aarch64_reg_t reg, i32_t imm)
{
	const aarch64_t imask	= 0b01110001000000000000000000011111;

	return (reg << 5) | aarch64_imm(imm) | imask;
}

aarch64_t aarch64_cmpx(aarch64_reg_t reg, i32_t imm)
{
	return aarch64_cmpw(reg, imm) | (1 << 31);
}

static void err_range(i64_t imm, i64_t min, i64_t max, char *ty)
{
	fprintf(stderr, "%li: immidiate %s out of range [%li %li]\n", (long)imm, ty, (long)min, (long)max);
	exit(-1);
}
static void err_mod(i64_t imm, u64_t mul, char *ty)
{
	fprintf(stderr, "%li: immidiate %s out not a multiple of %lu\n", (long)imm, ty, (unsigned long)mul);
	exit(-1);

}
static aarch64_t aarch64_imm_address_mode(u32_t ins, i64_t imm)
{
	switch (ins) {
		case LDRX:
		case STRX:
			if (imm > 32760)
				err_range(imm, 0, 32760, "offset");
			if (imm % 8 != 0)
				err_mod(imm, 8, "offset");
			return (imm / 8) << 10;
		case LDRW:
		case STRW:
			if (imm > 16380)
				err_range(imm, 0, 16380, "offset");
			if (imm % 4 != 0)
				err_mod(imm, 4, "offset");
			return (imm / 4) << 10;
		case STRH:
		case LDRH:
		case LDRSH:
			if (imm > 8190)
				err_range(imm, 0, 8190, "offset");
			if (imm % 2 != 0)
				err_mod(imm, 2, "offset");
			return (imm / 2) << 10;
		case LDRB:
		case LDRSB:
			if (imm > 4095)
				err_range(imm, 0, 4095, "offset");
			return imm << 10;
		case LDRXP:
		case LDRWP:
		case LDRHP:
		case LDRBP:
		case LDRSHP:
		case LDRSBP:
		case STRXP:
		case STRWP:
		case STRHP:
		case STRBP:
			if (imm > 255 || imm < -256)
				err_range(imm, -256, 255, "index");
			return imm << 12;
		default:
			fprintf(stderr, "Unknown instruction\n");
			exit(-1);
	}
}

aarch64_t aarch64_ldr
(
	aarch64_load_t	ins,
	aarch64_reg_t	dst,
	aarch64_reg_t	addr,
	u32_t		pimm
)
{
	aarch64_t imm;

	imm = aarch64_imm_address_mode(ins, pimm);
	return (addr << 5) | dst | (ins << 2) | imm;
}

aarch64_t aarch64_str
(
	aarch64_store_t	ins,
	aarch64_reg_t	dst,
	aarch64_reg_t	addr,
	u32_t		pimm
)
{
	aarch64_t imm;

	imm = aarch64_imm_address_mode(ins, pimm);
	return (addr << 5) | dst | (ins << 2) | imm;
}
static aarch64_t aarch64_mov_(aarch64_reg_t dst, aarch64_reg_t src, u32_t word)
{
	const aarch64_t imask	= 0b00101010000000000000001111100000;

	return (src << 16) | dst | (imask | (word << 31));
}
aarch64_t aarch64_mov(aarch64_reg_t dst, aarch64_reg_t src)
{
	return aarch64_mov_(dst, src, 1);
}
aarch64_t aarch64_movw(aarch64_reg_t dst, aarch64_reg_t src)
{
	return aarch64_mov_(dst, src, 0);
}

static aarch64_t aarch64_movi_(aarch64_reg_t dst, u64_t imm, u32_t word)
{
	const aarch64_t imask	= 0b01010010100000000000000000000000;
	if (imm < 0x10000) {
		return (imask | (word << 31)) | (imm << 5) | dst;
	}
	imm >>= 16;
	if (imm < 0x10000)
		return ((imask | (word << 31)) ^ (1 << 21)) | (imm << 5) | dst;
	if ((imm & (~0 ^ 0xFFFF)) != imm) {
		fprintf(stderr, "value '0x%lX' cannot be moved in a single instruction\n", (unsigned long)imm << 16);
		exit(-1);
	}
	imm >>= 16;
	if (imm < 0x10000)
		return ((imask | (word << 31)) ^ (2 << 21)) | (imm << 5) | dst;
	if ((imm & (~0 ^ 0xFFFF)) != imm) {
		fprintf(stderr, "value '0x%lX' cannot be moved in a single instruction\n", (unsigned long)imm << 32);
		exit(-1);
	}
	imm >>= 16;
	if (imm < 0x10000)
		return ((imask | (word << 31)) ^ (3 << 21)) | (imm << 5) | dst;
	fprintf(stderr, "value '0x%lX' cannot be moved in a single instruction\n", (unsigned long)imm << 48);
	exit(-1);
}

aarch64_t aarch64_movi(aarch64_reg_t dst, u64_t imm)
{
	return aarch64_movi_(dst, imm, 1);
}
aarch64_t aarch64_movwi(aarch64_reg_t dst, u64_t imm)
{
	return aarch64_movi_(dst, imm, 0);
}

aarch64_t aarch64_add_imm(aarch64_reg_t dst, aarch64_reg_t src, i32_t imm)
{
	const aarch64_t imask	= 0b10010001000000000000000000000000;
	return (dst << 5) | dst | aarch64_imm(imm) | imask;
}

aarch64_t aarch64_sub_imm(aarch64_reg_t dst, aarch64_reg_t src, i32_t imm)
{
	const aarch64_t imask	= 0b11010001000000000000000000000000;
	return (dst << 5) | dst | aarch64_imm(imm) | imask;
}