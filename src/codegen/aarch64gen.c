#include "../dfa.h"
#include "../util/dla.h"


static void prange(FILE *out, int start, int end, int dst)
{
	if (start == end) {
		fprintf(out, "	cmp	w3, #%d\n", start);
		fprintf(out, "	b.eq	s%ds\n", dst + 1);
	} else {
		fprintf(out, "	cmp	w3, #%d\n", start);
		fprintf(out, "	b.lt	redo\n");
		fprintf(out, "	cmp	w3, #%d\n", end);
		fprintf(out, "	b.le	s%ds\n", dst + 1);
	}
}
static void dfa_codegen_asm_state(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	dla_t dsts[dfa->length];
	state_t *s;
	int rs, re;

	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);
	fprintf(out, "s%ds:\n", idx + 1);

	if (accepting) {
		fprintf(out, "	str	x2, [x1]\n");
	}

	if (!state_transition_inv(dfa, idx, dsts)) {
		for (i = 0; i < dfa->length; i++)
			bytes_deinit(&dsts[i]);
		fprintf(out, "	%s", accepting ? "	b	return\n" : "	b	redo\n");
		return;
	}


	if (idx == 0) {
		fprintf(out, "	add	x2, x2, #1\n");
		fprintf(out, "start:\n");
		fprintf(out, "	ldrsb	w3, [x2]\n");
	} else {
		fprintf(out, "	ldrsb	w3, [x2, #1]!\n");
	}

	for (i = 0; i < dfa->length; i++) {
		if (dsts[i].length != 0) {
			rs = re = 0;
			DLA_FOREACH(&dsts[i], u8_t, c, {
				if (rs == 0) {
					rs = re = c;
				} else if (c == re + 1) {
					++re;
				} else {
					prange(out, rs, re, i);
					rs = re = c;
				}
			});
			prange(out, rs, re, i);
		}
		bytes_deinit(&dsts[i]);
	}
	fprintf(out, "	b	%s\n", accepting ? "return" : "redo");
}


void codegen_aarch64(FILE *out, dfa_t *dfa, re_ast_t *ast, const char *endchars)
{
	int i;
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
	fprintf(out, "	ldr	x2, [x0]\n");
	fprintf(out, "	str	xzr, [x1]\n");
	fprintf(out, "	b	start\n");

	for (i = 0; i < dfa->length; i++)
		dfa_codegen_asm_state(out, dfa, ast, i);

	fprintf(out, "redo:\n");
	fprintf(out, "	ldr	x4, [x1]\n");
	fprintf(out, "	cmp	x4, #0\n");
	fprintf(out, "	b.eq	redo1bb\n");
	fprintf(out, "return:\n");
	fprintf(out, "	mov	x0, #1\n");
	fprintf(out, "	ret\n");
	fprintf(out, "redo1bb:\n");
	fprintf(out, "	ldrsb	w3, [x2]\n");
	fprintf(out, "	cmp	w3, #'\\0'\n");
	fprintf(out, "	b.eq	miss\n");
	for (i = 0; endchars[i] != '\0'; i++) {
		fprintf(out, "	cmp	w3, #0x%X\n", endchars[i]);
		fprintf(out, "	b.eq	miss\n");
	}

	fprintf(out, "	ldr	x4, [x0]\n");
	fprintf(out, "	add	x4, x4, #1\n");
	fprintf(out, "	str	x4, [x0]\n");
	fprintf(out, "	mov	x2, x4\n");
	fprintf(out, "	b	start\n");
	fprintf(out, "miss:\n");
	fprintf(out, "	mov	x0, xzr\n");
	fprintf(out, "	ret\n");
#if defined(__APPLE__) && defined(__MACH__)
	fprintf(out, "	.cfi_endproc\n");
#endif
}