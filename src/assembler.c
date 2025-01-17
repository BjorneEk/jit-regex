#include "assembler.h"
#include "dfa.h"
#include "../include/a64_abbr.h"
#include "util/dla.h"



DLA_GEN(, idep_t, deps, init, deinit, push)
DLA_GEN(, a64_t, mach, init, deinit, push, len, set)
DLA_GEN(, block_t, blocks, init, deinit, push, len)


idep_t branch_dep(dep_type_t ty, u64_t instruction_index, a64_cond_t cond, u32_t target_ident)
{
	return (idep_t) {.ty = ty, .instruction_index = instruction_index, .cond = cond, .target_ident = target_ident};
}

idep_t load_dep(dep_type_t ty, u64_t instruction_index, a64_reg_t dst_reg, u32_t target_ident)
{
	return (idep_t) {.ty = ty, .instruction_index = instruction_index, .dst_reg = dst_reg, .target_ident = target_ident};
}

void b_state(block_t *b, a64_cond_t cond, u32_t target_ident)
{
	deps_push(&b->deps,
	branch_dep(
		BRANCH_TO_STATE,
		mach_len(&b->instructions),
		cond,
		target_ident));
	mach_push(&b->instructions, 0);
}

void b_lbl(block_t *b, a64_cond_t cond, u32_t target_ident)
{
	deps_push(&b->deps,
	branch_dep(
		BRANCH_TO_LBL,
		mach_len(&b->instructions),
		cond,
		target_ident));
	mach_push(&b->instructions, 0);
}

void ld_state(block_t *b, a64_reg_t dst_reg, u32_t target_ident)
{
	deps_push(&b->deps,
	load_dep(
		LOAD_STATE_ADDRESS,
		mach_len(&b->instructions),
		dst_reg,
		target_ident));
	mach_push(&b->instructions, 0);
}

void ins(block_t *b, a64_t instr)
{
	mach_push(&b->instructions, instr);
}

void ld_lbl(block_t *b, a64_reg_t dst_reg, u32_t target_ident)
{
	deps_push(&b->deps,
	load_dep(
		LOAD_LBL_ADDRESS,
		mach_len(&b->instructions),
		dst_reg,
		target_ident));
	ins(b, 0);
}

void init_iblock(block_t *b, block_type_t ty, u32_t ident)
{
	b->ty = ty;
	b->ident = ident;
	deps_init(&b->deps, 3);
	mach_init(&b->instructions, 3);
}

void set_dblock(block_t *b, u32_t ident, u64_t length, u8_t *data)
{
	b->ident = ident;
	b->data = data;
	b->length = length;
}

void link(dla_t *blocks)
{
	u64_t offset, max_offset;
	u32_t imm;
	bool pred;
	offset = 0;

	DLA_WHERE(blocks, block_t, block, block->ty == DBLOCK, {
		block->offset = offset;
		offset += block->length / 4 + 1;
	})

	DLA_WHERE(blocks, block_t, block, block->ty == IBLOCK, {
		block->offset = offset;
		offset += mach_len(&block->instructions);
	})
	max_offset = offset;
	DLA_FOREACH(blocks, block_t, block, {
		DLA_FOREACH(&block.deps, idep_t, dep, {
			pred = dep.ty == BRANCH_TO_STATE || dep.ty == LOAD_STATE_ADDRESS;
			DLA_WHERE(blocks, block_t, b_,
				pred && b_->ty == SBLOCK || !pred && b_->ty != SBLOCK,
				if (b_->ident == dep.target_ident) {
					offset = b_->offset;
					break;
			})

			if (offset == max_offset) {
				fprintf(stderr, "failed to link, could not find ident: %u\n", dep.target_ident);
				exit(-1);
			}

			imm = offset - (block.offset + dep.instruction_index);
			mach_set(&block.instructions, dep.instruction_index, dep.ty == BRANCH_TO_STATE || dep.ty == BRANCH_TO_LBL ?
				a64_b(dep.cond, imm) :
				a64_adr(dep.dst_reg, imm));
		})
		deps_deinit(&block.deps);
	})
}

enum count_matches_lbls {
	START,
	FAST,
	SLOW,
	MISS,
	MATCH
};
#define CNT (R0)
#define PTR (R1)
#define LEN (R2)
#define LAST_MATCH (R3)
#define CHAR (R4)

void gen_state_seq(dla_t *blocks, dfa_t *dfa, u64_t idx, u64_t maxlen, u64_t minlen, bool accepting);
void gen_state(dla_t *blocks, dfa_t *dfa, u64_t idx, u64_t maxlen, u64_t minlen, bool accepting)
{
	block_t b;
	u64_t i;
	dla_t dsts[dfa->length];


	init_iblock(&b, SBLOCK, idx);

	if (accepting)
		ins(&b, MOV(LAST_MATCH, PTR));

	if (!state_transition_inv(dfa, idx, dsts)) {
		for (i = 0; i < dfa->length; i++)
			bytes_deinit(&dsts[i]);
		if (accepting)
			b_lbl(&b, B, MISS);
		blocks_push(blocks, b);
		return;
	}
	ins(&b, LDRB(POST, CHAR, PTR, 1));
}
/*


*/
void gen_blocks_count_matches(dla_t *blocks, dfa_t *dfa, re_ast_t *ast)
{
	block_t b;
	u64_t i;
	init_iblock(&b, IBLOCK, START);
	ins(&b, MOV(LEN, R1));
	ins(&b, MOV(PTR, R0));
	ins(&b, MOV(CNT, XZR));
	ins(&b, MOV(LAST_MATCH, XZR));

	ins(&b, CMPI(LEN, 64));
	b_lbl(&b, LT, SLOW);
	b_lbl(&b, B, SLOW);
	blocks_push(blocks, b);
	init_iblock(&b, IBLOCK, FAST);

	for (i = 0; i < dfa->length; ++i) {
		if (dfa->states[i].is_dead)
			continue;
		if (dfa->states[i].is_seq)
			gen_state_seq(
				blocks,
				dfa,
				i,
				ast->max_length,
				ast->min_length,
				is_accepting_state(ast, &dfa->states[i]));
		else
			gen_state(
				blocks,
				dfa,
				i,
				ast->max_length,
				ast->min_length,
				is_accepting_state(ast, &dfa->states[i]));
	}

}