#include "aarch64_jit.h"

#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#if defined(__APPLE__) && defined(__MACH__)
	#include <libkern/OSCacheControl.h>
#endif

CODEGEN_DLA(aarch64_branch_t, branches)
CODEGEN_DLA(aarch64_t, a64)
CODEGEN_DLA(aarch64_block_t, blocks)


static inline aarch64_branch_t sbranch(aarch64_cond_t cond, u64_t ins, u32_t dst)
{
	return (aarch64_branch_t) {
		.ty = STATE,
		.cond = cond,
		.instruction_idx = ins,
		.destination_block_index = dst
	};
}
static inline aarch64_branch_t branch_other(aarch64_cond_t cond, u64_t ins, aarch64_branch_type_t ty)
{
	return (aarch64_branch_t) {
		.ty = ty,
		.cond = cond,
		.instruction_idx = ins,
		.destination_block_index = 0
	};
}

static inline aarch64_t *addi(aarch64_block_t *b, aarch64_t ins)
{
	a64_append(&b->instructions, ins);
	return &((aarch64_t*)(b->instructions.data))[b->instructions.length - 1];
}

static void seti(aarch64_block_t *b, aarch64_t ins, u64_t idx)
{
	*a64_getptr(&b->instructions, idx) = ins;
}

static inline void add_sbranch(aarch64_block_t *b, aarch64_cond_t ty, u32_t dst)
{
	branches_append(&b->branches, sbranch(ty, addi(b, 0) - ((aarch64_t*)b->instructions.data), dst));
}
static inline void add_gbranch(aarch64_block_t *b, aarch64_cond_t cond, aarch64_branch_type_t ty)
{
	branches_append(&b->branches, branch_other(cond, addi(b, 0) - ((aarch64_t*)b->instructions.data), ty));
}

static void add_range(aarch64_block_t *b, int start, int end, int dst, bool accepting)
{
	addi(b, aarch64_cmpw(R3, start));
	if (start == end) {
		add_sbranch(b, EQ, dst);
		return;
	}
	add_gbranch(b, LT, accepting ? END : REDO);
	addi(b, aarch64_cmpw(R3, end));
	add_sbranch(b, LS, dst);
}

static void init_block(aarch64_block_t *b, u32_t idx)
{
	b->idx = idx;
	branches_init(&b->branches, 10);
	a64_init(&b->instructions, 10);
}

static void gen_state(aarch64_mach_t *m, dfa_t *dfa, re_ast_t *ast, int idx, int *block_idx)
{
	aarch64_block_t	b;
	dla_t		dsts[dfa->length];
	state_t		*s;
	int 	i;
	int 	rs, re;
	bool 	accepting;

	init_block(&b, idx);

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	if (accepting)
		addi(&b, aarch64_str(STRX, R2, R1, 0));

	if (!state_transition_inv(dfa, idx, dsts)) {
		for (i = 0; i < dfa->length; i++)
			bytes_deinit(&dsts[i]);
		if (accepting)
			add_gbranch(&b, B, accepting ? END : REDO);
		blocks_append(&m->blocks, b);
		return;
	}

	if (idx == 0) {
		addi(&b, aarch64_add_imm(R2, R2, 1)); // cut of block to form start block
		blocks_append(&m->blocks, b);
		init_block(&b, *block_idx);
		m->start = *(block_idx);
		(*block_idx)++;

		addi(&b, aarch64_ldr(LDRSB, R3, R2, 0));
	} else {
		addi(&b, aarch64_ldr(LDRSBP, R3, R2, 1));
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

static u32_t find_offset(aarch64_mach_t *m, u32_t block_idx)
{
	int i;
	aarch64_block_t *b;
	for (i = 0; i < m->blocks.length; i++) {
		b = blocks_getptr(&m->blocks, i);
		if (b->idx == block_idx)
			return b->offset;
	}
	fprintf(stderr, "Failed to find block offset of block with index: %u\n", block_idx);
	exit(-1);
}

static void solve_state(aarch64_mach_t *m, aarch64_branch_t *b)
{
	switch (b->ty) {
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

static void link_mach(aarch64_mach_t *m)
{
	aarch64_block_t *b;
	aarch64_branch_t *branch;
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
			seti(b, aarch64_branch(branch->cond, imm), branch->instruction_idx);
		}
		branches_deinit(&b->branches);
	}
}

static void gen_mach(aarch64_mach_t *m, dfa_t *dfa, re_ast_t *ast, const char *endchars)
{
	int block_idx;
	aarch64_block_t b;
	int i;

	block_idx = dfa->length + 1;
	blocks_init(&m->blocks, 10);

	init_block(&b, block_idx);
	++block_idx;
	addi(&b, aarch64_ldr(LDRX, R2, R0, 0));
	addi(&b, aarch64_str(STRX, XZR, R1, 0));
	add_gbranch(&b, B, START);
	blocks_append(&m->blocks, b);

	for (i = 0; i < dfa->length; i++)
		gen_state(m, dfa, ast, i, &block_idx);

	m->redo = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, aarch64_ldr(LDRX, R4, R1, 0));
	addi(&b, aarch64_cmpx(R4, 0));
	add_gbranch(&b, EQ, REDO_INNER);
	blocks_append(&m->blocks, b);

	m->end = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, aarch64_movi(R0, 1));
	addi(&b, aarch64_ret());
	blocks_append(&m->blocks, b);

	m->redo_inner = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, aarch64_ldr(LDRSB, R3, R2, 0));
	addi(&b, aarch64_cmpw(R3, '\0'));
	add_gbranch(&b, EQ, MISS);
	for (i = 0; endchars[i] != '\0'; i++) {
		addi(&b, aarch64_cmpw(R3, endchars[i]));
		add_gbranch(&b, EQ, MISS);
	}
	addi(&b, aarch64_ldr(LDRX, R4, R0, 0));
	addi(&b, aarch64_add_imm(R4, R4, 1));
	addi(&b, aarch64_str(STRX, R4, R0, 0));
	addi(&b, aarch64_mov(R2, R4));
	add_gbranch(&b, B, START);
	blocks_append(&m->blocks, b);

	m->miss = block_idx;
	init_block(&b, block_idx);
	++block_idx;
	addi(&b, aarch64_mov(R0, XZR));
	addi(&b, aarch64_ret());
	blocks_append(&m->blocks, b);

	link_mach(m);
}
#if defined(__linux__)
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
static void aarch64_prog_init(aarch64_prog_t *prog, size_t size)
{
	size_t page_size;
	size_t size_;

	page_size = sysconf(_SC_PAGESIZE);
	size_  = ((size * sizeof(u32_t)) / page_size) + page_size;

	prog->code = mmap(
		NULL,
		size_,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0);

	if (prog->code == MAP_FAILED) {
		fprintf(stderr, "mmap failed (%s)\n", strerror(errno));
		exit(1);
	}

	prog->size = size;
}
void aarch64_jit(aarch64_prog_t *prog, dfa_t *dfa, re_ast_t *ast, const char *endchars)
{
	aarch64_mach_t m;
	aarch64_block_t b;
	int i;

	gen_mach(&m, dfa, ast, endchars);
	aarch64_prog_init(prog, m.length * sizeof(aarch64_t));

	for (i = 0; i < m.blocks.length; i++) {
		b = blocks_get(&m.blocks, i);
		memcpy(&prog->code[b.offset], b.instructions.data, b.instructions.length * sizeof(aarch64_t));
		a64_deinit(&b.instructions);
	}
	blocks_deinit(&m.blocks);
}
#elif defined(__APPLE__) && defined(__MACH__)
void aarch64_jit(aarch64_prog_t *prog, dfa_t *dfa, re_ast_t *ast, const char *endchars)
{
	aarch64_mach_t m;
	aarch64_block_t b;
	size_t page_size;
	size_t size;
	int i;

	gen_mach(&m, dfa, ast, endchars);

	page_size = sysconf(_SC_PAGESIZE);
	size = ((m.length * sizeof(aarch64_t)) / page_size) + page_size;

	prog->code = mmap(
		NULL,
		size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT,
		-1,
		0
	);

	if (prog->code == MAP_FAILED) {
		fprintf(stderr, "mmap failed (%s)\n", strerror(errno));
		exit(-1);
	}

	for (i = 0; i < m.blocks.length; i++) {
		b = blocks_get(&m.blocks, i);
		memcpy(&prog->code[b.offset], b.instructions.data, b.instructions.length * sizeof(aarch64_t));
		a64_deinit(&b.instructions);
	}
	blocks_deinit(&m.blocks);

	if (mprotect(prog->code, size, PROT_READ | PROT_EXEC) == -1) {
		fprintf(stderr, "mprotect failed (%s)\n", strerror(errno));
		munmap(prog->code, size);
		exit(-1);
	}

	sys_icache_invalidate(prog->code, m.length * sizeof(aarch64_t));
}
#endif
void aarch64_prog_deinit(aarch64_prog_t *prog)
{

	size_t page_size;
	size_t size_;

	page_size = sysconf(_SC_PAGESIZE);
	size_ = ((prog->size * sizeof(u32_t)) / page_size) + page_size;
	munmap(prog->code, size_);
}

void aarch64_write_bin(FILE *fp, const aarch64_prog_t *prog)
{
	fwrite(prog->code, sizeof(aarch64_t), prog->size, fp);
}
