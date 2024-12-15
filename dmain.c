#include <stdio.h>
#include "src/regex.h"
#include "src/dfa.h"
#include "src/codegen/codegen.h"
#include "src/util/dla.h"
#include "src/aarch64/aarch64_ins.h"
#include "src/jit/aarch64_jit.h"
#include "src/util/mfile.h"

const char *float_re = "[0-9]+(.[0-9]*)?";

const char *c_float_re = "(([0-9]+.[0-9]*)|(.[0-9]+))(f|F)?";
const char *c_int_re = "(-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7])))";
const char *re_print = "print";

const char *c_int_re_ = "(-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7])|(b(0|1)+)))";

typedef struct part {
	char *start;
	size_t length;
} part_t;
typedef struct pfile {
	mfile_t file;
	dla_t parts;
} pfile_t;

//#define MIN_PART (100)
CODEGEN_DLA(part_t, parts)

bool __split_part(pfile_t *f, u32_t pidx)
{
	u32_t pos;
	part_t *p;

	p = parts_getptr(&f->parts, pidx);
	pos = p->length / 2;
	printf("part %d, len %lu\n", pidx, p->length);
	//if (split < MIN_PART)
	//	return false;

	while (pos > 0 && p->start[pos] != '\n')
		--pos;
	if (pos == 0)
		return false;

	parts_append(&f->parts, (part_t){
		.start = &p->start[pos + 1],
		.length = p->length - pos
	});
	p->length = pos;
	return true;
}
void __partition_file(const char *fname, pfile_t *f)
{

	mfile_open(&f->file, fname);

	parts_init(&f->parts, 4);

	parts_append(&f->parts, (part_t){
		.start = f->file.data,
		.length = f->file.size
	});

	__split_part(f, 0);
	__split_part(f, 0);
	__split_part(f, 1);
	printf("parts: %lu\n", f->parts.length);
	for (int i = 0; i < f->parts.length; i++) {
		part_t p = parts_get(&f->parts, i);
		printf("%.*s\n", (int)p.length, p.start);
	}
}

void test_regex(const char *re)
{
	re_ast_t ast;
	resz_t root;

	re_ast_init(&ast, 10);
	root = parse_regex(&ast, re);

	regex_print(&ast, root, false);

	re_ast_deinit(&ast);
}

void test_only_dfa(const char *re)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root);
	//regex_print(&ast, root, true);
	//codegen_c(stdout, &dfa, &ast, "\n");
	print_dfa(stdout, &dfa, &ast);

	re_ast_deinit(&ast);
	deinit_dfa(&dfa);
}

void test_dfa(const char *re, char *dotname, char *cname, char *sname)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;
	FILE *dot, *c, *s;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root);


	//print_dfa(stdout, &dfa, &ast);

	dot = fopen(dotname, "w");
	c = fopen(cname, "w");
	s = fopen(sname, "w");
	print_dfa_digraph(dot, &dfa, &ast);

	codegen_c(c, &dfa, &ast, "\n");
	codegen_aarch64(s, &dfa, &ast, "\n");

	fclose(dot);
	fclose(c);
	fclose(s);

	re_ast_deinit(&ast);
	deinit_dfa(&dfa);
}

CODEGEN_DLA(aarch64_t, arm)
void test_aarch64_asm(char *bin)
{
	dla_t code;
	FILE *fp;
	arm_init(&code, 10);

	arm_append(&code, aarch64_ldr(LDRX, R0, R1, 0));
	arm_append(&code, aarch64_ldr(LDRB, R4, R6, 0));
	arm_append(&code, aarch64_ldr(LDRSH, R20, R8, 4));
	arm_append(&code, aarch64_str(STRX, R0, R1, 0));
	arm_append(&code, aarch64_ldr(LDRSBP, R4, R2, 1));
	arm_append(&code, aarch64_str(STRX, XZR, R2, 16));
	arm_append(&code, aarch64_mov(R0, R1));
	arm_append(&code, aarch64_movw(R0, R1));
	arm_append(&code, aarch64_movi(R0, 1));
	arm_append(&code, aarch64_movi(R0, 0x10000));
	arm_append(&code, aarch64_movi(R0, 0x100000000));
	arm_append(&code, aarch64_movi(R0, 0x1000000000000));
	arm_append(&code, aarch64_movwi(R0, 1));
	arm_append(&code, aarch64_movwi(R0, 0x10000));
	arm_append(&code, aarch64_branch(NE, -4));
	arm_append(&code, aarch64_add_imm(R1, R1, 1));

	fp = fopen(bin, "w");

	fwrite(code.data, sizeof(aarch64_t), code.length, fp);
	fclose(fp);
	arm_deinit(&code);
}

void _grep(const char *fname, int (*lex)(const char**,const char**))
{
	const char *b, *e;
	mfile_t file;
	char buff[4096];
	mfile_open(&file, fname);
	b = file.data;
loop:
	if (lex(&b, &e)) {
		for(;;) {
			if (*e == '\n')
				break;
			if (*e == '\0')
				return;
			++e;
		}
		strncpy(buff, b, e - b);
		puts(buff);
		b = e + 1;
		goto loop;
	}
	for(;;) {
		if (*b == '\n')
			break;
		if (*b != '\0')
			return;
		++b;
	}
	++b;
	goto loop;
}

void test_grep(const char *re, const char *inname, const char *bin_out)
{

	re_ast_t ast;
	aarch64_prog_t prog;
	resz_t root;
	dfa_t dfa;
	FILE *bin;
	int (*lex)(const char**,const char**);

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);
	make_dfa(&dfa, &ast, root);
	aarch64_jit(&prog, &dfa, &ast, "\n");
	re_ast_deinit(&ast);
	deinit_dfa(&dfa);

	bin = fopen(bin_out, "w");
	aarch64_write_bin(bin, &prog);
	fclose(bin);

	lex = (int (*)(const char**,const char**))(uintptr_t)prog.code;
	_grep(inname, lex);
	aarch64_prog_deinit(&prog);

}

void test_jit(const char *re, const char *in, const char *bin_out)
{

	re_ast_t ast;
	aarch64_prog_t prog;
	resz_t root;
	dfa_t dfa;
	FILE *bin;
	const char *b, *e;
	int (*lex)(const char**,const char**);

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);
	make_dfa(&dfa, &ast, root);
	aarch64_jit(&prog, &dfa, &ast, "\n");
	re_ast_deinit(&ast);
	deinit_dfa(&dfa);

	bin = fopen(bin_out, "w");
	aarch64_write_bin(bin, &prog);
	fclose(bin);

	lex = (int (*)(const char**,const char**))(uintptr_t)prog.code;
	b = in;
	while (lex(&b, &e)) {
		printf("Match: [%lu - %lu] [%c - %c]\n", b - in, e - in, *b, *e);
		b = e + 1;
	}
	aarch64_prog_deinit(&prog);

}



int main(void)
{
	//test_regex(c_float_re);
	//test_regex(c_int_re);
	//pfile_t f;

	//test_dfa(c_float_re, "files/float.dot", "files/float.c", "files/float.S");
	//test_dfa(c_int_re,"files/int.dot", "files/int.c", "files/int.S");
	test_only_dfa("(hej)|(snopp)|[0-9]*");
	//test_dfa(re_print,"files/print.dot", "files/print.c", "files/print.S");
	//test_aarch64_asm("files/jit.bin");
	//test_jit(c_int_re,"0xFEADBEEF 07 -10 hgh", "files/int.bin");
	//test_grep("print", "files/print.in", "files/print.out");
	//partition_file("files/partition_test.in", &f);
	return 0;
}
