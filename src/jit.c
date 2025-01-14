#include "jit.h"

#include <unistd.h>

CODEGEN_DLA(branch_t, branches)
CODEGEN_DLA(a64_t, a64)
CODEGEN_DLA(block_t, blocks)


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

static void add_range(block_t *b, int start, int end, int dst, bool accepting)
{
	addi(b, a64_cmpiw(R3, start));
	if (start == end) {
		add_sbranch(b, EQ, dst);
		return;
	}
	add_gbranch(b, LT, accepting ? END : REDO);
	addi(b, a64_cmpiw(R3, end));
	add_sbranch(b, LS, dst);
}

static void init_block(block_t *b, u32_t idx)
{
	b->idx = idx;
	branches_init(&b->branches, 10);
	a64_init(&b->instructions, 10);
}

static void gen_state(mach_t *m, dfa_t *dfa, re_ast_t *ast, int idx, int *block_idx)
{
	block_t	b;
	dla_t		dsts[dfa->length];
	state_t		*s;
	int 	i;
	int 	rs, re;
	bool 	accepting;

	init_block(&b, idx);

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	if (accepting)
		addi(&b, a64_load_store(STORE, IMM, SZX, 0, R2, R1, 0));

	if (!state_transition_inv(dfa, idx, dsts)) {
		for (i = 0; i < dfa->length; i++)
			bytes_deinit(&dsts[i]);
		if (accepting)
			add_gbranch(&b, B, accepting ? END : REDO);
		blocks_append(&m->blocks, b);
		return;
	}

	if (idx == 0) {
		addi(&b, a64_addi(R2, R2, 1)); // cut of block to form start block
		blocks_append(&m->blocks, b);
		init_block(&b, *block_idx);
		m->start = *(block_idx);
		(*block_idx)++;

		addi(&b, a64_load_store(LOAD, IMM, SZB, 1, R3, R2, 0));
	} else {
		addi(&b, a64_load_store(LOAD, POST, SZB, 1, R3, R2, 1));
	}

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
				add_range(&b, rs, re, i, accepting);
				rs = re = c;
			}
		});
		add_range(&b, rs, re, i, accepting);
iter_end:
		bytes_deinit(&dsts[i]);
	}
	add_gbranch(&b, B, accepting ? END : REDO);
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
}

static void solve_state(mach_t *m, branch_t *b)
{
	switch (b->ty) {
		case BCMP:
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
	branch_t *branch;
	u64_t offset;
	u64_t instruction_offset, destination_offset, imm;
	int i, j;

	offset = 0;
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
	}
}

static void gen_mach(mach_t *m, dfa_t *dfa, re_ast_t *ast, const char *endchars)
{
	int block_idx;
	block_t b;
	int i;

	block_idx = dfa->length + 1;
	blocks_init(&m->blocks, 10);

	init_block(&b, block_idx);
	++block_idx;
	addi(&b, a64_load_store(LOAD, IMM, SZX, 0, R2, R0, 0));
	addi(&b, a64_load_store(STORE, IMM, SZX, 0, XZR, R1, 0));
	add_gbranch(&b, B, START);
	blocks_append(&m->blocks, b);

	for (i = 0; i < dfa->length; i++)
		gen_state(m, dfa, ast, i, &block_idx);

	m->redo = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, a64_load_store(LOAD, IMM, SZX, 0, R4, R1, 0));
	addi(&b, a64_cmpi(R4, 0));
	add_gbranch(&b, EQ, REDO_INNER);
	blocks_append(&m->blocks, b);

	m->end = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, a64_movi(R0, 1));
	addi(&b, a64_ret());
	blocks_append(&m->blocks, b);

	m->redo_inner = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, a64_load_store(LOAD, IMM, SZB, 1, R3, R2, 0));
	addi(&b, a64_cmpiw(R3, '\0'));
	add_gbranch(&b, EQ, MISS);
	for (i = 0; endchars[i] != '\0'; i++) {
		addi(&b, a64_cmpiw(R3, endchars[i]));
		add_gbranch(&b, EQ, MISS);
	}
	addi(&b, a64_load_store(LOAD, IMM, SZX, 0, R4, R0, 0));
	addi(&b, a64_addi(R4, R4, 1));
	addi(&b, a64_load_store(STORE, IMM, SZX, 0, R4, R0, 0));
	addi(&b, a64_mov(R2, R4));
	add_gbranch(&b, B, START);
	blocks_append(&m->blocks, b);

	m->miss = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, a64_mov(R0, XZR));
	addi(&b, a64_ret());
	blocks_append(&m->blocks, b);

	link_mach(m);
}

void jit(a64_jit_t *prog, dfa_t *dfa, re_ast_t *ast, const char *endchars)
{
	mach_t m;
	block_t b;
	int i;

	gen_mach(&m, dfa, ast, endchars);

	a64_jit_init(prog, m.length + 10);
	for (i = 0; i < m.blocks.length; i++) {
		b = blocks_get(&m.blocks, i);
		memcpy(&prog->code[b.offset], b.instructions.data, b.instructions.length * sizeof(a64_t));
		a64_deinit(&b.instructions);
	}
	prog->length = m.length;
	blocks_deinit(&m.blocks);
}