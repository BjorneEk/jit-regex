#include "jit.h"

#include <string.h>
#include <unistd.h>

CODEGEN_DLA(branch_t, branches)
CODEGEN_DLA(a64_t, a64)
CODEGEN_DLA(block_t, blocks)
CODEGEN_DLA(data_ptr_t, dptrs)
CODEGEN_DLA(data_block_t, dblocks)

static inline branch_t sbranch(a64_cond_t cond, u64_t ins, u32_t dst)
{
	return (branch_t) {
		.ty = STATE,
		.cond = cond,
		.instruction_idx = ins,
		.destination_block_index = dst
	};
}
static inline branch_t branch_other(a64_cond_t cond, u64_t ins, branch_type_t ty)
{
	return (branch_t) {
		.ty = ty,
		.cond = cond,
		.instruction_idx = ins,
		.destination_block_index = 0
	};
}

static inline a64_t *addi(block_t *b, a64_t ins)
{
	a64_append(&b->instructions, ins);
	return &((a64_t*)(b->instructions.data))[b->instructions.length - 1];
}

static void seti(block_t *b, a64_t ins, u64_t idx)
{
	*a64_getptr(&b->instructions, idx) = ins;
}

static inline void add_sbranch(block_t *b, a64_cond_t ty, u32_t dst)
{
	branches_append(&b->branches, sbranch(ty, addi(b, 0) - ((a64_t*)b->instructions.data), dst));
}
static inline void add_gbranch(block_t *b, a64_cond_t cond, branch_type_t ty)
{
	branches_append(&b->branches, branch_other(cond, addi(b, 0) - ((a64_t*)b->instructions.data), ty));
}
static inline void add_dptr(block_t *b, u32_t data_block_idx)
{
	dptrs_append(&b->data_ptrs, (data_ptr_t){.ty=DATA_PTR, .instruction_idx=addi(b, 0) - ((a64_t*)b->instructions.data), .idx=data_block_idx});
}
static inline void add_sptr(block_t *b, u32_t state_idx)
{
	dptrs_append(&b->data_ptrs, (data_ptr_t){.ty=STATE_PTR, .instruction_idx=addi(b, 0) - ((a64_t*)b->instructions.data), .idx=state_idx});
}
static void add_range(block_t *b, int start, int end, int dst)
{
	addi(b, a64_cmpiw(R3, start));
	if (start == end) {
		add_sbranch(b, EQ, dst);
		return;
	}
	addi(b, a64_b(LT, 3));
	addi(b, a64_cmpiw(R3, end));
	add_sbranch(b, LE, dst);
}

static void init_block(block_t *b, u32_t idx)
{
	b->idx = idx;
	branches_init(&b->branches, 10);
	dptrs_init(&b->data_ptrs, 10);
	a64_init(&b->instructions, 10);
}
static void shift_block(mach_t *m, block_t *b, int *idx)
{
	init_block(b, *idx);
	++*idx;
}

static void first_block(mach_t *m, dfa_t *dfa, block_t *b, int *idx)
{
	*idx = dfa->length + 1;
	init_block(b, *idx);
	++*idx;
}
static u64_t add_dblock(mach_t *m, u8_t *data, u64_t length)
{
	data_block_t b;

	b.data = malloc(length);
	b.length = length;
	memcpy(b.data, data, length);
	dblocks_append(&m->data_blocks, b);
	return m->data_blocks.length - 1;
}

CODEGEN_DLA(dfa_seq_t, seq_list)
static void gen_small_seq(block_t *b, u64_t length, u32_t dst)
{
	if (length == 1) {
		addi(b, a64_load_store(LOAD, POST, SZB, 0, R3, R1, 1));
		addi(b, a64_load_store(LOAD, POST, SZB, 0, R5, R4, 1));
		addi(b, a64_cmpw(R3, R5));
		add_sbranch(b, EQ, dst);
		return;
	}
	if (length < 4) {
		addi(b, a64_load_store(LOAD, POST, SZH, 0, R3, R1, 2));
		addi(b, a64_load_store(LOAD, POST, SZH, 0, R5, R4, 2));
		addi(b, a64_cmpw(R3, R5));
		if (length == 2) {
			add_sbranch(b, EQ, dst);
		} else {
			addi(b, a64_b(EQ, 3));
			addi(b, a64_subi(R1, R1, 1));
			add_gbranch(b, B, MISS);
			gen_small_seq(b, length - 2, dst);
		}
		return;
	}
	if (length < 8) {
		addi(b, a64_load_store(LOAD, POST, SZW, 0, R3, R1, 4));
		addi(b, a64_load_store(LOAD, POST, SZW, 0, R5, R4, 4));
		addi(b, a64_cmpw(R3, R5));
		if (length == 4) {
			add_sbranch(b, EQ, dst);
		} else {
			addi(b, a64_b(EQ, 3));
			addi(b, a64_subi(R1, R1, 3));
			add_gbranch(b, B, MISS);
			gen_small_seq(b, length - 4, dst);
		}
		return;
	}
	if (length < 16) {
		addi(b, a64_load_store(LOAD, POST, SZX, 0, R3, R1, 8));
		addi(b, a64_load_store(LOAD, POST, SZX, 0, R5, R4, 8));
		addi(b, a64_cmpw(R3, R5));
		if (length == 8) {
			add_sbranch(b, EQ, dst);
		} else {
			addi(b, a64_b(EQ, 3));
			addi(b, a64_subi(R1, R1, 7));
			add_gbranch(b, B, MISS);
			gen_small_seq(b, length - 8, dst);
		}
		return;
	}
	if (length < 32) {
		addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 1, R0, R4));
		addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 1, R1, R1));

		addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R1, R1, R0));
		addi(b, a64_simd_uminv(SIMD_B, SIMD_FULL, R0, R1));
		addi(b, a64_simd_umov(SIMD_B, 0, R3, R0));
		addi(b, a64_cmpiw(R3, 0xFF));
		if (length == 16) {
			add_sbranch(b, EQ, dst);
		} else {
			addi(b, a64_b(EQ, 3));
			addi(b, a64_subi(R1, R1, 15));
			add_gbranch(b, B, MISS);
			gen_small_seq(b, length - 16, dst);
		}
		return;
	}
	if (length < 48) {
		addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 2, R1, R4));
		addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 2, R3, R1));

		addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R1, R1, R3));
		addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R2, R2, R4));

		addi(b, a64_simd_and(SIMD_FULL, R1, R1, R2));

		addi(b, a64_simd_uminv(SIMD_B, SIMD_FULL, R0, R1));
		addi(b, a64_simd_umov(SIMD_B, 0, R3, R0));
		addi(b, a64_cmpiw(R3, 0xFF));
		if (length == 32) {
			add_sbranch(b, EQ, dst);
		} else {
			addi(b, a64_b(EQ, 3));
			addi(b, a64_subi(R1, R1, 31));
			add_gbranch(b, B, MISS);
			gen_small_seq(b, length - 32, dst);
		}
		return;
	}
	if (length < 64) {
		addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 3, R1, R4));
		addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 3, R4, R1));

		addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R1, R1, R4));
		addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R2, R2, R5));
		addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R3, R3, R6));

		addi(b, a64_simd_and(SIMD_FULL, R1, R1, R2));
		addi(b, a64_simd_and(SIMD_FULL, R1, R1, R3));

		addi(b, a64_simd_uminv(SIMD_B, SIMD_FULL, R0, R1));
		addi(b, a64_simd_umov(SIMD_B, 0, R3, R0));
		addi(b, a64_cmpiw(R3, 0xFF));
		if (length == 48) {
			add_sbranch(b, EQ, dst);
		} else {
			addi(b, a64_b(EQ, 3));
			addi(b, a64_subi(R1, R1, 47));
			add_gbranch(b, B, MISS);
			gen_small_seq(b, length - 48, dst);
		}
		return;
	}
	addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 4, R1, R4));
	addi(b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 4, R5, R1));

	addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R1, R1, R5));
	addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R2, R2, R6));
	addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R3, R3, R7));
	addi(b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R4, R4, R8));

	addi(b, a64_simd_and(SIMD_FULL, R1, R1, R2));
	addi(b, a64_simd_and(SIMD_FULL, R1, R1, R3));
	addi(b, a64_simd_and(SIMD_FULL, R1, R1, R4));

	addi(b, a64_simd_uminv(SIMD_B, SIMD_FULL, R0, R1));
	addi(b, a64_simd_umov(SIMD_B, 0, R3, R0));
	addi(b, a64_cmpiw(R3, 0xFF));
	if (length == 48) {
		add_sbranch(b, EQ, dst);
	} else {
		addi(b, a64_b(EQ, 3));
		addi(b, a64_subi(R1, R1, 63));
		add_gbranch(b, B, MISS);
		gen_small_seq(b, length - 48, dst);
	}
}
static void mov_l(block_t *b, a64_reg_t rdst, a64_reg_t rinter, u64_t val)
{
	addi(b, a64_movi(rdst, 0));
	addi(b, a64_movi(rinter, (0xFFFFL << 0) & val));
	if (val <= 0xFFFF)
		return;
	addi(b, a64_add(rdst, rdst, rinter));
	addi(b, a64_movi(rinter, (0xFFFFL << 16) & val));
	if (val <= 0xFFFFFFFF)
		return;
	addi(b, a64_add(rdst, rdst, rinter));
	addi(b, a64_movi(rinter, (0xFFFFL << 32) & val));
	if (val <= 0xFFFFFFFFFFFF)
		return;
	addi(b, a64_add(rdst, rdst, rinter));
	addi(b, a64_movi(rinter, (0xFFFFL << 48) & val));
}
/*
static void gen_s0(mach_t *m, dfa_t *dfa, re_ast_t *ast, int idx, int *block_idx, int *need_cmp)
{
	block_t b;
	state_t *s;
	dfa_seq_t *seq;
	u8_t *data;
	u64_t length;

	s = &dfa->states[idx];
	init_block(&b, idx);
	seq = seq_list_getptr(&dfa->seqs, s->seq_idx);
	data = seq->seq.data;
	length = seq->seq.length;


	addi(&b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 4, R1, R1));
	addi(&b, a64_simd_movib(SIMD_FULL, R0, 0));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R1, R1, R0));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R2, R2, R0));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R3, R3, R0));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R4, R4, R0));
}
*/
static void gen_seq_state(mach_t *m, dfa_t *dfa, re_ast_t *ast, int idx, int *block_idx, int *need_cmp)
{
	block_t	b;
	state_t		*s;
	dfa_seq_t	*seq;
	u8_t *data;
	u64_t length;
	u16_t raw;
	u64_t bidx;

	s = &dfa->states[idx];
	init_block(&b, idx);
	seq = seq_list_getptr(&dfa->seqs, s->seq_idx);
	data = seq->seq.data;
	length = seq->seq.length;



	if (length > 2) {
		bidx = add_dblock(m, data, length);
		add_dptr(&b, bidx);
	}
	if (length > 128) {
		*need_cmp = 1;
		mov_l(&b, R6, R8, length);
		add_sptr(&b, seq->next - 1);
		add_gbranch(&b, B, CMP);
		blocks_append(&m->blocks, b);
		return;
	}
	if (length > 2) { // 2 byte can be moved as immidiate
		gen_small_seq(&b, length, seq->next - 1);
		add_gbranch(&b, B, MISS);
		blocks_append(&m->blocks, b);
		return;
	}
	raw = *data;
	addi(&b, a64_load_store(LOAD, POST, SZH, 0, R3, R1, 2));

	if ((raw & 0xFFF) == raw) { // cmpiw can compare 12 bit immidates
		addi(&b, a64_cmpiw(R3, raw));
	} else { //
		addi(&b, a64_moviw(R5, raw));
		addi(&b, a64_cmpw(R3, R5));
	}
	add_sbranch(&b, EQ, seq->next - 1);
	addi(&b, a64_subi(R1, R1, 1));
	add_gbranch(&b, B, MISS);
	blocks_append(&m->blocks, b);
}

static void gen_state(mach_t *m, dfa_t *dfa, re_ast_t *ast, int idx, int *block_idx, int *need_cmp)
{
	block_t	b;
	dla_t		dsts[dfa->length];
	state_t		*s;
	int 	i;
	int 	rs, re;
	bool 	accepting;



	s = &dfa->states[idx];
	if (s->is_dead)
		return;
	if (s->is_seq) {
		gen_seq_state(m, dfa, ast, idx, block_idx, need_cmp);
		return;
	}


	init_block(&b, idx);
	accepting = is_accepting_state(ast, s);

	if (accepting)
		addi(&b, a64_mov(R2, R1));

	if (!state_transition_inv(dfa, idx, dsts)) {
		for (i = 0; i < dfa->length; i++)
			bytes_deinit(&dsts[i]);
		if (accepting)
			add_gbranch(&b, B, MISS);
		blocks_append(&m->blocks, b);
		return;
	}

	addi(&b, a64_load_store(LOAD, POST, SZB, 0, R3, R1, 1));

	for (i = 0; i < dfa->length; i++) {
		if (dsts[i].length == 0)
			goto iter_end;
		rs = re = 0;
		DLA_FOREACH(&dsts[i], u8_t, c, {
			if (rs == 0) {
				rs = re = c;
			} else if (c == re + 1) {
				++re;
			} else {
				add_range(&b, rs, re, i);
				rs = re = c;
			}
		});
		add_range(&b, rs, re, i);
iter_end:
		bytes_deinit(&dsts[i]);
	}
	add_gbranch(&b, B, MISS);
	blocks_append(&m->blocks, b);
}

static u32_t find_offset(mach_t *m, u32_t block_idx)
{
	int i;
	block_t *b;
	for (i = 0; i < m->blocks.length; i++) {
		b = blocks_getptr(&m->blocks, i);
		if (b->idx == block_idx)
			return b->offset;
	}
	fprintf(stderr, "Failed to find block offset of block with index: %u\n", block_idx);
	exit(-1);
	//return 0;
}

static void solve_state(mach_t *m, branch_t *b)
{
	switch (b->ty) {
		case CMP:
			b->destination_block_index = m->mcmp;
			break;
		case END:
			b->destination_block_index = m->end;
			break;
		case START:
			b->destination_block_index = m->start;
			break;
		case REDO:
			b->destination_block_index = m->redo;
			break;
		case REDO_INNER:
			b->destination_block_index = m->redo_inner;
			break;
		case MISS:
			b->destination_block_index = m->miss;
			break;
		case STATE:
			break;
	}
	b->ty = STATE;
}

static void link_mach(mach_t *m)
{
	block_t *b;
	data_block_t *db;
	branch_t *branch;
	data_ptr_t *dptr;
	u64_t offset;
	u64_t instruction_offset, destination_offset, imm;
	int i, j;

	offset = 0;

	for (i = 0; i < m->data_blocks.length; i++) {
		db = dblocks_getptr(&m->data_blocks, i);
		db->offset = offset;
		offset += db->length / 4 + 1;
	}
	for (i = 0; i < m->blocks.length; i++) {
		b = blocks_getptr(&m->blocks, i);
		b->offset = offset;
		offset += b->instructions.length;
	}
	m->length = offset;
	for (i = 0; i < m->blocks.length; i++) {
		b = blocks_getptr(&m->blocks, i);
		for (j = 0; j < b->branches.length; j++) {
			branch = branches_getptr(&b->branches, j);
			if (branch->ty != STATE)
				solve_state(m, branch);
			instruction_offset = b->offset + branch->instruction_idx;
			destination_offset = find_offset(m, branch->destination_block_index);
			imm = destination_offset - instruction_offset;

			seti(b, a64_b(branch->cond, imm), branch->instruction_idx);
		}
		branches_deinit(&b->branches);
		for (j = 0; j < b->data_ptrs.length; j++) {
			dptr = dptrs_getptr(&b->data_ptrs, j);

			instruction_offset = b->offset + dptr->instruction_idx;
			if (dptr->ty == STATE_PTR) {
				destination_offset = find_offset(m, dptr->idx);
				imm = destination_offset - instruction_offset;
				seti(b, a64_adr(R7, imm), dptr->instruction_idx);
			} else {
				destination_offset = dblocks_get(&m->data_blocks, dptr->idx).offset;
				imm = destination_offset - instruction_offset;
				//printf("pid: %u, doff: 0x%llX, ioff: 0x%llX\n", dptr->idx, destination_offset*4, instruction_offset*4);
				seti(b, a64_adr(R4, imm), dptr->instruction_idx);
			}
		}
		dptrs_deinit(&b->data_ptrs);
	}
}



static void gen_mach(mach_t *m, dfa_t *dfa, re_ast_t *ast)
{
	int block_idx;
	block_t b;
	int i;
	int need_cmp;
	need_cmp = 0;
	block_idx = dfa->length + 1;
	blocks_init(&m->blocks, 10);
	dblocks_init(&m->data_blocks, 10);
	first_block(m, dfa, &b, &block_idx);

	addi(&b, a64_mov(R1, R0));
	addi(&b, a64_mov(R0, XZR));
	blocks_append(&m->blocks, b);
	m->start = block_idx;
	shift_block(m, &b, &block_idx);
	addi(&b, a64_mov(R2, XZR));
	addi(&b, a64_load_store(LOAD, IMM, SZB, 0, R3, R1, 0));
	addi(&b, a64_cmpiw(R3, '\0'));
	add_gbranch(&b, EQ, END);
	blocks_append(&m->blocks, b);

	for (i = 0; i < dfa->length; i++)
		gen_state(m, dfa, ast, i, &block_idx, &need_cmp);

	m->miss = block_idx;
	shift_block(m, &b, &block_idx);
	addi(&b, a64_cmp(R2, XZR));
	add_gbranch(&b, EQ, START);
	addi(&b, a64_addi(R0, R0, 1));
	add_gbranch(&b, B, START);
	blocks_append(&m->blocks, b);

	m->mcmp = block_idx;
	if (!need_cmp)
		goto no_cmp;
	shift_block(m, &b, &block_idx);

	addi(&b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 4, R1, R4));
	addi(&b, a64_simd_ld1(LD1_IMM, SIMD_B, SIMD_FULL, 4, R5, R1));
	addi(&b, a64_subi(R6, R6, 64));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R1, R1, R5));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R2, R2, R6));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R3, R3, R7));
	addi(&b, a64_simd_cmeq(SIMD_B, SIMD_FULL, R4, R4, R8));

	addi(&b, a64_simd_and(SIMD_FULL, R1, R1, R2));
	addi(&b, a64_simd_and(SIMD_FULL, R1, R1, R3));
	addi(&b, a64_simd_and(SIMD_FULL, R1, R1, R4));

	addi(&b, a64_simd_uminv(SIMD_B, SIMD_FULL, R0, R1));
	addi(&b, a64_simd_umov(SIMD_B, 0, R3, R0));
	addi(&b, a64_cmpiw(R3, 0xFF));
	addi(&b, a64_b(NE, 11));
	addi(&b, a64_cmpi(R6, 64));
	addi(&b, a64_b(NE, -15));
	addi(&b, a64_load_store(LOAD, POST, SZB, 0, R3, R1, 1));
	addi(&b, a64_load_store(LOAD, POST, SZB, 0, R5, R4, 1));
	addi(&b, a64_subi(R6, R6, 1));
	addi(&b, a64_cmp(R3, R5));
	add_gbranch(&b, NE, MISS);
	addi(&b, a64_cmp(R6, XZR));
	addi(&b, a64_b(NE, -7));
	addi(&b, a64_br(R7));
	addi(&b, a64_add(R1, R1, 63));
	add_gbranch(&b, B, MISS);
	blocks_append(&m->blocks, b);
no_cmp:
	m->end = block_idx;
	shift_block(m, &b, &block_idx);
	addi(&b, a64_ret());
	blocks_append(&m->blocks, b);

	link_mach(m);
}

uintptr_t jit_count_matches(a64_jit_t *prog, dfa_t *dfa, re_ast_t *ast)
{
	mach_t m;
	block_t b;
	data_block_t db;
	int i;
	uintptr_t ep;

	gen_mach(&m, dfa, ast);

	a64_jit_init(prog, m.length + 10);
	for (i = 0; i < m.data_blocks.length; i++) {
		db = dblocks_get(&m.data_blocks, i);
		//printf("off: %llu\n",db.offset);
		memcpy(&prog->code[db.offset], db.data, db.length);
		free(db.data);
	}
	for (i = 0; i < m.blocks.length; i++) {
		b = blocks_get(&m.blocks, i);
		if (i == 0)
			ep = (uintptr_t)(prog->code + b.offset);
		memcpy(&prog->code[b.offset], b.instructions.data, b.instructions.length * sizeof(a64_t));
		a64_deinit(&b.instructions);
	}
	prog->length = m.length;
	blocks_deinit(&m.blocks);
	dblocks_deinit(&m.data_blocks);
	return ep;
}