#include "regex.h"
#include "util/dla.h"
#include "dfa.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <unistd.h>

static bool nullable(re_ast_t *ast, resz_t n)
{
	switch (re_type(ast, n)) {
		case RE_OR: return nullable(ast, *re_child(ast, n, 0)) || nullable(ast, *re_child(ast, n, 1));
		case RE_CAT: return nullable(ast, *re_child(ast, n, 0)) && nullable(ast, *re_child(ast, n, 1));
		case RE_STAR: return true;
		case RE_CHAR: return false;
		case RE_ACCEPT: return false;
		case RE_NULL: return true;
		default:
			return false;
	}
}

static void union_(resz_t *res, resz_t *len_res, const resz_t *a, resz_t len_a, const resz_t *b, resz_t len_b)
{
	int i = 0;
	int j = 0;
	int k = 0;

	while (i < len_a && j < len_b) {
		if (a[i] < b[j]) {
			res[k++] = a[i++];
		} else if (a[i] > b[j]) {
			res[k++] = b[j++];
		} else {
			res[k++] = a[i++];
			j++;
		}
	}

	if (i < len_a) {
		memmove(&res[k], &a[i], (len_a - i) * sizeof(resz_t));
		k += len_a - i;
	}

	if (j < len_b) {
		memmove(&res[k], &b[j], (len_b - j) * sizeof(resz_t));
		k += len_b - j;
	}

	*len_res = k;
}

static void setfirst(re_ast_t *ast, resz_t n)
{
	resz_t _first[256];
	resz_t *first = &_first[1];

	switch (re_type(ast, n)) {
		case RE_OR:
			setfirst(ast, *re_child(ast, n, 0));
			setfirst(ast, *re_child(ast, n, 1));
			union_(
				first,
				_first,
				re_first(ast, *re_child(ast, n, 0)),
				re_firstlen(ast, *re_child(ast, n, 0)),
				re_first(ast, *re_child(ast, n, 1)),
				re_firstlen(ast, *re_child(ast, n, 1)));
			break;
		case RE_CAT:
			setfirst(ast, *re_child(ast, n, 0));
			setfirst(ast, *re_child(ast, n, 1));
			if (nullable(ast, *re_child(ast, n, 0))) {
				union_(
					first,
					_first,
					re_first(ast, *re_child(ast, n, 0)),
					re_firstlen(ast, *re_child(ast, n, 0)),
					re_first(ast, *re_child(ast, n, 1)),
					re_firstlen(ast, *re_child(ast, n, 1)));
			} else {
				_first[0] = re_firstlen(ast, *re_child(ast, n, 0));
				memmove(first, re_first(ast, *re_child(ast, n, 0)), _first[0] * sizeof(resz_t));
			}
			break;
		case RE_STAR:
			setfirst(ast, *re_child(ast, n, 0));
			_first[0] = re_firstlen(ast, *re_child(ast, n, 0));
			memmove(first, re_first(ast, *re_child(ast, n, 0)), _first[0] * sizeof(resz_t));
			break;
		case RE_ACCEPT:
		case RE_CHAR:
			_first[0] = 1;
			first[0] = n;
			break;
		case RE_NULL:
			_first[0] = 0;
			break;
	}
	ast->tree[n].first = re_add_data(ast, _first, _first[0]);
}

static void setlast(re_ast_t *ast, resz_t n)
{
	resz_t _last[256];
	resz_t *last = &_last[1];

	switch (re_type(ast, n)) {
		case RE_OR:
			setlast(ast, *re_child(ast, n, 0));
			setlast(ast, *re_child(ast, n, 1));
			union_(
				last,
				_last,
				re_last(ast, *re_child(ast, n, 0)),
				re_lastlen(ast, *re_child(ast, n, 0)),
				re_last(ast, *re_child(ast, n, 1)),
				re_lastlen(ast, *re_child(ast, n, 1)));
			break;
		case RE_CAT:
			setlast(ast, *re_child(ast, n, 0));
			setlast(ast, *re_child(ast, n, 1));
			if (nullable(ast, *re_child(ast, n, 1))) {
				union_(
					last,
					_last,
					re_last(ast, *re_child(ast, n, 0)),
					re_lastlen(ast, *re_child(ast, n, 0)),
					re_last(ast, *re_child(ast, n, 1)),
					re_lastlen(ast, *re_child(ast, n, 1)));
			} else {
				_last[0] = re_lastlen(ast, *re_child(ast, n, 1));
				memmove(last, re_last(ast, *re_child(ast, n, 1)), _last[0] * sizeof(resz_t));
			}
			break;
		case RE_STAR:
			setlast(ast, *re_child(ast, n, 0));
			_last[0] = re_lastlen(ast, *re_child(ast, n, 0));
			memmove(last, re_last(ast, *re_child(ast, n, 0)), _last[0] * sizeof(resz_t));
			break;
		case RE_ACCEPT:
		case RE_CHAR:
			_last[0] = 1;
			last[0] = n;
			break;
		case RE_NULL:
			_last[0] = 0;
			break;
	}
	ast->tree[n].last = re_add_data(ast, _last, _last[0]);
}

static void _setfollows(re_ast_t *ast, resz_t root)
{
	int i, j;
	resz_t *l1, *l2;
	switch (re_type(ast, root)) {
		case RE_CAT:
			l1 = re_last(ast, *re_child(ast, root, 0));
			l2 = re_first(ast, *re_child(ast, root, 1));

			for (i = 0; i < re_lastlen(ast, *re_child(ast, root, 0)); i++) {
				for (j = 0; j < re_firstlen(ast, *re_child(ast, root, 1)); j++) {
					re_add_follows(ast, l1[i], l2[j]);
				}
			}
			_setfollows(ast, *re_child(ast, root, 0));
			_setfollows(ast, *re_child(ast, root, 1));
			break;
		case RE_OR:
			_setfollows(ast, *re_child(ast, root, 0));
			_setfollows(ast, *re_child(ast, root, 1));
			break;
		case RE_STAR:
			l1 = re_last(ast, *re_child(ast, root, 0));
			l2 = re_first(ast, *re_child(ast, root, 0));

			for (i = 0; i < re_lastlen(ast, *re_child(ast, root, 0)); i++) {
				for (j = 0; j < re_firstlen(ast, *re_child(ast, root, 0)); j++) {
					re_add_follows(ast, l1[i], l2[j]);
				}
			}
			_setfollows(ast, *re_child(ast, root, 0));
			//_setfollows(ast, *re_child(ast, root, 1));
			break;
		default:break;
	}
}

static void setfollows(re_ast_t *ast, resz_t root)
{
	re_init_follows(ast);
	_setfollows(ast, root);
}

void dfa_init(dfa_t *dfa, int init)
{
	dfa->allocated = init;
	dfa->length = 0;
	dfa->states = malloc(init * sizeof(state_t));
}

static int dfa_newstate(dfa_t *dfa)
{
	int res;

	if (dfa->length + 1 >= dfa->allocated) {
		dfa->allocated += 3;
		dfa->states = realloc(dfa->states, dfa->allocated * sizeof(state_t));
	}
	res = dfa->length;
	++dfa->length;
	return res;
}

static void get_sc(re_ast_t *ast, state_t *S, char c, resz_t *sc, int *sc_len)
{
	int i, len;
	resz_t n;

	len = 0;
	for (i = 0; i < S->length; i++) {
		n = S->restates[i];
		if (re_type(ast, n) == RE_CHAR && re_char(ast, n) == c) {
			sc[len++] = n;
		}
	}
	*sc_len = len;
}

static void insert_sorted_unique(resz_t *arr, int *len, resz_t val)
{
	int pos = 0;

	while (pos < *len && arr[pos] < val) {
		pos++;
	}

	if (pos < *len && arr[pos] == val)
		return;

	memmove(&arr[pos + 1], &arr[pos], (*len - pos) * sizeof(resz_t));

	arr[pos] = val;

	(*len)++;
}
static void get_follows(re_ast_t *ast, resz_t *follows, int *follows_len, resz_t *sc, int sc_len)
{
	int i, j;
	szbuff_t *f;

	for (i = 0; i < sc_len; i++) {
		f = re_follows(ast, sc[i]);
		for (j = 0; j < f->len; j++)
			insert_sorted_unique(follows, follows_len, f->data[j]);
	}
}

static void parr(FILE *out, resz_t *arr, resz_t n);
static int get_state(dfa_t *dfa, resz_t *re, int re_len)
{
	int i;

	for (i = 0; i < dfa->length; i++) {
		if (re_len == dfa->states[i].length &&
			memcmp(re, dfa->states[i].restates, re_len * sizeof(resz_t)) == 0) {
			return i + 1;
		}
	}
	return 0;
}

static void dfagen(dfa_t *dfa, re_ast_t *ast, int state)
{
	int i;
	state_t *S_;
	resz_t sc[256], follows[256];
	int sc_len, follows_len;
	int s_, S_idx;


	for (i = 0; i < 256; i++) {
		sc_len = 0;
		follows_len = 0;
		get_sc(ast, &dfa->states[state], i, sc, &sc_len);
		get_follows(ast, follows, &follows_len, sc, sc_len);
		s_ = get_state(dfa, follows, follows_len);
		if (s_ == 0 && follows_len > 0) {
			S_idx = dfa_newstate(dfa);
			S_ = &dfa->states[S_idx];
			S_->length = follows_len;
			S_->restates = calloc(follows_len, sizeof(resz_t));
			memmove(S_->restates, follows, follows_len * sizeof(resz_t));
			memset(S_->transition, 0, sizeof(S_->transition));
			dfa->states[state].transition[i] = S_idx + 1;
			dfagen(dfa, ast, S_idx);
		} else {
			dfa->states[state].transition[i] = follows_len > 0 ? s_ : 0;
		}
	}
}

void make_dfa(dfa_t *dfa, re_ast_t *ast, resz_t root)
{
	resz_t S0_len;
	state_t *S0;
	int S0_idx;

	setfirst(ast, root);
	setlast(ast, root);
	setfollows(ast, root);
	dfa_init(dfa, 5);

	S0_idx = dfa_newstate(dfa);
	S0 = &dfa->states[S0_idx];

	S0_len = re_firstlen(ast, root);
	S0->restates = calloc(S0_len, sizeof(resz_t));
	memmove(S0->restates, re_first(ast, root), S0_len * sizeof(resz_t));
	S0->length = S0_len;
	memset(S0->transition, 0, sizeof(S0->transition));

	dfagen(dfa, ast, S0_idx);
}

void deinit_dfa(dfa_t *dfa)
{
	int i;

	for (i = 0; i < dfa->length; i++)
		free(dfa->states[i].restates);
	free(dfa->states);
}

bool is_accepting_state(re_ast_t *ast, state_t *s)
{
	int i;

	for (i = 0; i < s->length; i++) {
		if (re_type(ast, s->restates[i]) == RE_ACCEPT)
			return true;
	}
	return false;
}

static void parr(FILE *out, resz_t *arr, resz_t n)
{
	int i;
	fprintf(out, "{");
	for (i = 0; i < n - 1; i++)
		fprintf(out, "%d, ", arr[i]);
	fprintf(out, "%d}", arr[n - 1]);
}

static void print_state(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	state_t *s;


	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	if (idx == 0)
		fprintf(out, "Start S1: ");
	else
		fprintf(out, "S%d: ", idx + 1);
	parr(out, s->restates, s->length);
	fprintf(out, " %c", accepting ? '[' : '{');

	for (i = 0; i < 256; i++) {
		if (s->transition[i] != 0) {
			if (isprint((unsigned char)i)) {
				fprintf(out, "(%c -> S%d)", i, s->transition[i]);
			} else {
				fprintf(out, "(0x%X -> S%d)", i, s->transition[i]);
			}
		}
	}
	fprintf(out, "%c\n", accepting ? ']' : '}');
}

static void print_state_dot(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	state_t *s;


	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	for (i = 0; i < 256; i++) {
		if (s->transition[i] != 0) {
			if (isprint((unsigned char)i)) {
				fprintf(out, "	S%d -> S%d [label=\"%c\"];\n", idx + 1, s->transition[i], i);
			} else {
				fprintf(out, "	S%d -> S%d [label=\"0x%X\"];\n", idx + 1, s->transition[i], i);
			}
		}
	}
	if (accepting)
		fprintf(out, "S%d [shape=doublecircle];\n", idx + 1);
}

void print_dfa(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i;
	for (i = 0; i < dfa->length; i++) {
		print_state(out, dfa, ast, i);
	}
}

void print_dfa_digraph(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i;
	printf("generated digraph, view digraph on 'https://dreampuf.github.io/GraphvizOnline'\n");
	fprintf(out, "digraph StateMachine {\n	start -> S1\n");
	for (i = 0; i < dfa->length; i++) {
		print_state_dot(out, dfa, ast, i);
	}
	fprintf(out, "}");
}

//CODEGEN_DLA(u8_t, bytes)

bool state_transition_inv(dfa_t *dfa, int idx, dla_t *dsts)
{
	int i;
	bool used;
	state_t *s;

	s = &dfa->states[idx];
	for (i = 0; i < dfa->length; i++)
		bytes_init(&dsts[i], 10);
	used = 0;
	for (i = 0; i < 256; i++)
		if (s->transition[i] != 0) {
			used = true;
			bytes_append(&dsts[s->transition[i] - 1], (u8_t)i);
		}
	return used;
}

/*
typedef struct tf {
	u8_t *inputs;
	resz_t ninputs;
} tfinv_t;


CODEGEN_DLA(u8_t, bytes)

static void get_state_tfinv(dfa_t *dfa, int idx, dla_t  *dsts)
{
	int i;
	state_t *s;

	s = &dfa->states[idx];
	for (i = 0; i < dfa->length; i++)
		bytes_init(&dsts[i], 10);

	for (i = 0; i < 256; i++)
		if (s->transition[i] != 0) {
			bytes_append(&dsts[s->transition[i] - 1], (u8_t)i);
		}
}

#define AS_B "x2"
#define AS_END "x1"
#define AS_CH "w4"
#define AS_START "x0"
static void prange(FILE *out, int start, int end, int dst, int *uid)
{
	if (start == end) {
		fprintf(out, "	cmp	%s, #%d\n", AS_CH, start);
		fprintf(out, "	b.eq	S%dS\n", dst + 1);
	} else {
		fprintf(out, "	cmp	%s, #%d\n", AS_CH, start);
		fprintf(out, "	b.lo	BB%d\n", *uid);
		fprintf(out, "	cmp	%s, #%d\n", AS_CH, end);
		fprintf(out, "	b.ls	S%dS\n", dst + 1);
		fprintf(out, "BB%d:\n", *uid);
		*uid += 1;
	}
}
static void dfa_codegen_asm_state(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx, int *bbs)
{
	bool accepting;
	int i, j;
	dla_t  dsts[dfa->length];
	state_t *s;
	int rs, re;

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);
	fprintf(out, "S%dS:\n", idx + 1);

	if (accepting) {
		fprintf(out, "	str	%s, [%s]\n", AS_B, AS_END);
	}
	fprintf(out, "	add	%s, %s, #1\n", AS_B, AS_B);

	if (idx == 0)
		fprintf(out, "start:\n");

	fprintf(out, "	ldrsb	%s, [%s]\n", AS_CH, AS_B);

	get_state_tfinv(dfa, idx, dsts);
	for (i = 0; i < dfa->length; i++) {
		if (dsts[i].length != 0) {
			rs = re = 0;
			DLA_FOREACH(&dsts[i], u8_t, c, {
				if (rs == 0) {
					rs = re = c;
				} else if (c == re + 1) {
					++re;
				} else {
					prange(out, rs, re, i, bbs);
					rs = re = c;
				}
			});
			prange(out, rs, re, i, bbs);
		}
		bytes_deinit(&dsts[i]);
	}
	fprintf(out, "	b	%s\n", accepting ? "end" : "REDO");
}

static void dfa_codegen_c_state_(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i, j;
	dla_t  dsts[dfa->length];
	state_t *s;

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);
	fprintf(out, "S%d:\n", idx + 1);

	if (accepting)
		fprintf(out, "	*end = b;\n");
	fprintf(out, "	++b;\n");

	if (idx == 0)
		fprintf(out, "start:\n");

	fprintf(out, "	switch (*b) {\n");

	get_state_tfinv(dfa, idx, dsts);
	for (i = 0; i < dfa->length; i++) {
		if (dsts[i].length != 0) {
			DLA_FOREACH(&dsts[i], u8_t, c, {

				fprintf(out, "		case 0x%X:", c);
				if (isprint(c))
					fprintf(out, "  // '%c'", c);
				fprintf(out, "\n");
			})
			fprintf(out, "			goto S%d;\n", i + 1);
		}
		bytes_deinit(&dsts[i]);
	}
	fprintf(out, "		default:\n			%s;\n	}\n", accepting ? "return" : "goto REDO");
}
static void dfa_codegen_c_state(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	state_t *s;

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);
	fprintf(out, "S%d:\n", idx + 1);

	if (accepting)
		fprintf(out, "	*end = b;\n");
	fprintf(out, "	++b;\n");

	if (idx == 0)
		fprintf(out, "start:\n");

	fprintf(out, "	switch (*b) {\n");

	for (i = 0; i < 256; i++) {
		if (s->transition[i] != 0) {
			fprintf(out, "		case 0x%X:", i);
			if (isprint(i))
				fprintf(out, "  // '%c'", i);
			fprintf(out, "\n			goto S%d;\n", s->transition[i]);
		}
	}
	fprintf(out, "		default:\n			%s;\n	}\n", accepting ? "return" : "goto REDO");
}

void dfa_codegen_asm(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i, bbs;
#if defined(__linux__)
	fprintf(out, "	.section .text\n");
	fprintf(out, "	.global lex\n");
	fprintf(out, "lex:\n");
#elif defined(__APPLE__) && defined(__MACH__)
	fprintf(out, "	.section	__TEXT,__text,regular,pure_instructions\n");
	fprintf(out, "	.p2align	2\n");
	fprintf(out, "	.globl	_lex\n");
	fprintf(out, "_lex:\n");
	fprintf(out, "	.cfi_startproc\n");
#endif
	fprintf(out, "	ldr	%s, [%s]\n", AS_B, AS_START);
	fprintf(out, "	str	xzr, [%s]\n", AS_END);
	fprintf(out, "	b	start\n");
	bbs = 0;
	for (i = 0; i < dfa->length; i++)
		dfa_codegen_asm_state(out, dfa, ast, i, &bbs);

	fprintf(out, "REDO:\n");
	fprintf(out, "	ldr	x6, [%s]\n", AS_END);
	fprintf(out, "	cmp	x6, #0\n");
	fprintf(out, "	b.ne	end\n");
	fprintf(out, "	cmp	%s, #'\\0'\n", AS_CH);
	fprintf(out, "	b.eq	end\n");
	fprintf(out, "	cmp	%s, #'\\n'\n", AS_CH);
	fprintf(out, "	b.eq	end\n");
	fprintf(out, "	ldr	x6, [%s]\n", AS_START);
	fprintf(out, "	add	x6, x6, #1\n");
	fprintf(out, "	str	x6, [%s]\n", AS_START);
	fprintf(out, "	mov	%s, x6\n", AS_B);
	fprintf(out, "	b	start\n");
	fprintf(out, "end:\n");
	fprintf(out, "	ret\n");
#if defined(__APPLE__) && defined(__MACH__)
	fprintf(out, "	.cfi_endproc\n");
#endif
}

void dfa_codegen_c(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i;
	fprintf(out, "void lex(const char **begin, const char **end)\n{\n");
	fprintf(out, "	const char *b = *begin;\n	*end = ((void*)0);\n	goto start;\n");

	for (i = 0; i < dfa->length; i++) {
		dfa_codegen_c_state_(out, dfa, ast, i);
	}
	fprintf(out, "REDO:\n	if (*end != ((void*)0))\n	return;\n");
	fprintf(out, "	switch (*b) {\n		case '\\0':\n		case '\\n':\n			return;\n");
	fprintf(out, "	}\n	++(*begin);\n	b = *begin;\n	goto start;\n}");
}






typedef enum arm_branch_type {
	ARM_EQ = 0,
	ARM_NE = 0b1,
	ARM_LO = 0b11,
	ARM_LS = 0b1001,
	ARM_B,
	ARM_END,
	ARM_REDO,
	ARM_START,
	ARM_NE_END,
	ARM_EQ_END
} arm_branch_type_t;

typedef struct arm_branch {
	arm_branch_type_t ty;
	arm_t *branch;
	u32_t dst_block;
} arm_branch_t;

typedef struct arm_ir_block {
	u64_t offset;
	dla_t instrs;
	dla_t branches;
	u32_t idx;
} arm_ir_block_t;

typedef struct arm_ir {
	dla_t blocks;
	u32_t start;
	u32_t end;
	u32_t redo;
	u32_t length;
} arm_ir_t;

CODEGEN_DLA(arm_branch_t, branches)
CODEGEN_DLA(arm_t, arm)
CODEGEN_DLA(arm_ir_block_t, blocks)

static arm_branch_t arm_branch(arm_branch_type_t ty, arm_t *ins, u32_t dst)
{
	return (arm_branch_t) {
		.ty = ty,
		.dst_block = dst,
		.branch = ins
	};
}
static arm_t *last_ins(arm_ir_block_t *b)
{
	return &((arm_t*)b->instrs.data)[b->instrs.length - 1];
}

static void arm_init_block(arm_ir_block_t *b, u32_t idx)
{
	b->idx = idx;
	branches_init(&b->branches, 10);
	arm_init(&b->instrs, 10);
}

#define CMPW_IMM_OFF (10)
static arm_t cmp_w4_imm(u32_t val)
{
	const arm_t imask		= 0b01110001000000000000000010011111;
	const arm_t getbits_mask	= 0b00000000001111111111110000000000;
	if (val > 4095) {
		fprintf(stderr, "cmp w4, #0x%X: to large immidiate value\n", val);
		exit(-1);
	}
	return imask | (val << CMPW_IMM_OFF) & getbits_mask;
}

#define B_EQ_OFF (5)
static arm_t arm_b_cond(i32_t offset, arm_branch_type_t cond)
{
	const arm_t imask		= 0b1010100000000000000000000000000;
	const arm_t getbits_mask	= 0b0000011111111111111111111100000;
	if (offset >= 0)
		return imask | ((offset) << B_EQ_OFF) & getbits_mask | cond;
	return imask | ((offset & 0x7FFFF) << B_EQ_OFF) & getbits_mask | cond;
}

static arm_t arm_b_uncond(i32_t offset)
{
	const arm_t imask		= 0b00010100000000000000000000000000;
	const arm_t getbits_mask	= 0b00000011111111111111111111111111;
	if (offset >= 0)
		return imask | ((offset)) & getbits_mask;
	return imask | (offset & getbits_mask);
}
static u32_t arm_get_offset(arm_ir_t *ir, u32_t idx)
{
	int i;
	arm_ir_block_t *b;
	for (i = 0; i < ir->blocks.length; i++) {
		b = &((arm_ir_block_t*)(ir->blocks.data))[i];
		if (b->idx == idx)
			return b->offset;
	}
	fprintf(stderr, "could not find arm basic block with offset: %u\n", idx);
	exit(-1);
}
static arm_t arm_gen_branch(arm_ir_t *ir, arm_branch_t br, u32_t ins_off, u32_t dst_off)
{
	i32_t offset;

	switch (br.ty) {
		case ARM_EQ:
		case ARM_NE:
		case ARM_LO:
		case ARM_LS:
			return arm_b_cond(dst_off - ins_off, br.ty);
		case ARM_B:
			return arm_b_uncond(dst_off - ins_off);
		case ARM_END:
			return arm_b_uncond(arm_get_offset(ir, ir->end) - ins_off);
		case ARM_REDO:
			return arm_b_uncond(arm_get_offset(ir, ir->redo) - ins_off);
		case ARM_START:
			return arm_b_uncond(arm_get_offset(ir, ir->start) - ins_off);
		case ARM_NE_END:
			return arm_b_cond(arm_get_offset(ir, ir->end) - ins_off, ARM_NE);
		case ARM_EQ_END:
			return arm_b_cond(arm_get_offset(ir, ir->end) - ins_off, ARM_EQ);
			break;
	}
}

static void arm_add_range(arm_ir_t *ir, arm_ir_block_t *b, int start, int end, int dst, int *block_idx)
{
	if (start == end) {
		arm_append(&b->instrs,
			cmp_w4_imm(start));
		arm_append(&b->instrs, 0);
		branches_append(&b->branches,
			arm_branch(ARM_EQ, last_ins(b), dst));
	} else {
		arm_append(&b->instrs,
			cmp_w4_imm(start));

		arm_append(&b->instrs, 0);
		branches_append(&b->branches,
			arm_branch(ARM_LO, last_ins(b), *block_idx));

		arm_append(&b->instrs,
			cmp_w4_imm(end));
		arm_append(&b->instrs, 0);

		branches_append(&b->branches,
			arm_branch(ARM_LS, last_ins(b), dst));

		blocks_append(&ir->blocks, *b);
		arm_init_block(b, *block_idx);
		++*block_idx;
	}
}
*/
/*
parametrized:
	cmp	w4, #%d, int
	b.eq	lbl
	b.ne	lbl
	b.lo	lbl
	b.ls	lbl
	b	lbl
static:

	str	x2, [x1]	-> 0xf9000022
	add	x2, x2, #1	-> 0x91000442
	ldrsb	w4, [x2]	-> 0x39c00044
	ldr	x2, [x0]	-> 0xf9400002
	str	xzr, [x1]	-> 0xf900003f
	ldr	x6, [x1]	-> 0xf9400026
	cmp	x6, #0		-> 0xf10000df
	cmp	w4, #'\\n'	-> 0x7100289f
	cmp	w4, #'\\0'	-> 0x7100c09f
	ldr	x6, [x0]	-> 0xf9400006
	add	x6, x6, #1	-> 0x910004c6
	str	x6, [x0]	-> 0xf9000006
	mov	x2, x6		-> 0xaa0603e2
	ret			-> 0xd65f03c0
 */
/*
typedef enum arm_static_instr {
	STR_X2_aX1	= 0xf9000022,
	ADD_X2_X2_1	= 0x91000442,
	LDRSB_W4_aX2	= 0x39c00044,
	LDR_X2_aX0	= 0xf9400002,
	STR_XZR_aX1	= 0xf900003f,
	LDR_X6_aX1	= 0xf9400026,
	CMP_X6_0	= 0xf10000df,
	CMP_W4_NL	= 0x7100289f,
	CMP_W4_0	= 0x7100c09f,
	LDR_X6_aX0	= 0xf9400006,
	ADD_X6_X6_1	= 0x910004c6,
	STR_X6_aX0	= 0xf9000006,
	MOV_X2_X6	= 0xaa0603e2,
	RET		= 0xd65f03c0
} arm_static_instr_t;


block_idx starts at dfa length to not have collissions
*/
/*
static void gen_arm_ir_state(arm_ir_t *ir, dfa_t *dfa, re_ast_t *ast, int idx, int *block_idx)
{
	bool accepting;
	int i, j;
	dla_t  dsts[dfa->length];
	state_t *s;
	int rs, re;
	arm_ir_block_t block;


	arm_init_block(&block, idx);
	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	if (accepting)
		arm_append(&block.instrs,
		STR_X2_aX1);
	arm_append(&block.instrs,
		ADD_X2_X2_1);

	if (idx == 0) {
		blocks_append(&ir->blocks, block);
		arm_init_block(&block, *block_idx);
		ir->start = *block_idx;
		++*block_idx;
	}

	arm_append(&block.instrs,
		LDRSB_W4_aX2);

	get_state_tfinv(dfa, idx, dsts);

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
				arm_add_range(ir, &block, rs, re, i, block_idx);
				rs = re = c;
			}
		});
		arm_add_range(ir, &block, rs, re, i, block_idx);
iter_end:
		bytes_deinit(&dsts[i]);
	}
	arm_append(&block.instrs, 0);
	branches_append(&block.branches,
		arm_branch(accepting ? ARM_END : ARM_REDO, last_ins(&block), 0));
	blocks_append(&ir->blocks, block);
}

static void dfa_irgen_arm(arm_ir_t *ir, dfa_t *dfa, re_ast_t *ast, bool brake_on_newline)
{
	int block_idx;
	arm_ir_block_t block, *b;
	int i, j, offset;

	block_idx = dfa->length + 1;

	arm_init_block(&block, block_idx);
	++block_idx;

	arm_append(&block.instrs,
		LDR_X2_aX0);
	arm_append(&block.instrs,
		STR_XZR_aX1);
	arm_append(&block.instrs, 0);
	branches_append(&block.branches,
		arm_branch(ARM_START, last_ins(&block), 0));

	blocks_append(&ir->blocks, block);

	for (i = 0; i < dfa->length; i++) {
		gen_arm_ir_state(ir, dfa, ast, i, &block_idx);
	}
	ir->redo = block_idx;
	arm_init_block(&block, block_idx);
	++block_idx;

	arm_append(&block.instrs,
		LDR_X6_aX1);

	arm_append(&block.instrs,
		CMP_X6_0);
	arm_append(&block.instrs, 0);
	branches_append(&block.branches,
		arm_branch(ARM_NE_END, last_ins(&block), 0));

	arm_append(&block.instrs,
		CMP_W4_0);
	arm_append(&block.instrs, 0);
	branches_append(&block.branches,
		arm_branch(ARM_EQ_END, last_ins(&block), 0));

	if (brake_on_newline) {
		arm_append(&block.instrs,
			CMP_W4_NL);
		arm_append(&block.instrs, 0);
		branches_append(&block.branches,
			arm_branch(ARM_EQ_END, last_ins(&block), 0));
	}

	arm_append(&block.instrs,
		LDR_X6_aX0);
	arm_append(&block.instrs,
		ADD_X6_X6_1);
	arm_append(&block.instrs,
		STR_X6_aX0);
	arm_append(&block.instrs,
		MOV_X2_X6);

	arm_append(&block.instrs, 0);
	branches_append(&block.branches,
		arm_branch(ARM_START, last_ins(&block), 0));

	blocks_append(&ir->blocks, block);
	ir->end = block_idx;
	arm_init_block(&block, block_idx);
	++block_idx;

	arm_append(&block.instrs,
		RET);

	blocks_append(&ir->blocks, block);

	offset = 0;
	for (i = 0; i < ir->blocks.length; i++) {
		b = &((arm_ir_block_t*)(ir->blocks.data))[i];
		b->offset = offset;
		offset += b->instrs.length;
	}
	ir->length = offset;
	for (i = 0; i < ir->blocks.length; i++) {
		b = &((arm_ir_block_t*)(ir->blocks.data))[i];
		arm_branch_t *br;
		for (j = 0; j < b->branches.length; j++) {
			br = &((arm_branch_t*)(b->branches.data))[j];
			u32_t ins_off = b->offset + (br->branch - (arm_t*)b->instrs.data);
			u32_t dst_off = arm_get_offset(ir, br->dst_block);
		//	printf("bi: %lu\n", br->branch - (arm_t*)b->instrs.data);
			*br->branch = arm_gen_branch(ir, *br, ins_off, dst_off);
		}

		//b->offset = offset;
		//offset += b->instrs.length;
	}
}


static void exec_alloc(arm_exec_t *exec, size_t size)
{
	size_t page_size;
	size_t size_;

	page_size = sysconf(_SC_PAGESIZE);
	size_  = (((size * sizeof(u32_t)) + page_size - 1) / page_size) * page_size;

	exec->code = mmap(
		NULL,
		size_,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0);

	if (exec->code == MAP_FAILED) {
		fprintf(stderr, "exec_alloc failed");
		exit(1);
	}

	exec->size = size;
}
arm_exec_t dfa_jit_compile(dfa_t *dfa, re_ast_t *ast, bool brake_on_newline)
{
	arm_ir_t ir;
	arm_ir_block_t *b;
	arm_exec_t res;
	int i;

	blocks_init(&ir.blocks, 10);
	dfa_irgen_arm(&ir, dfa, ast, brake_on_newline);

	exec_alloc(&res, ir.length);
	//printf("len: %u\n", ir.length);
	for (i = 0; i < ir.blocks.length; i++) {
		b = &((arm_ir_block_t*)(ir.blocks.data))[i];
		//printf("i: %d, off: %llu\n", i, b->offset);
		//for (int j = 0; j < b->instrs.length; j++)
		//	printf("%llu	0x%X\n", (b->offset + j) * 4, ((arm_t*)(b->instrs.data))[j]);
		memcpy(&res.code[b->offset], b->instrs.data, b->instrs.length * sizeof(arm_t));
		arm_deinit(&b->instrs);
		branches_deinit(&b->branches);
	}
	blocks_deinit(&ir.blocks);
	return res;
}
*/