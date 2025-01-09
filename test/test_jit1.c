#include <stdio.h>
#include "../src/regex.h"
#include "../src/dfa.h"
#include "../src/jit.h"
#include "common/test.h"

void test_jit(const char *re, const char *out)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;
	FILE *outf;
	a64_jit_t prog;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root, DFA_OPT_NONE);
	//regex_print(&ast, root, true);
	//codegen_c(stdout, &dfa, &ast, "\n");
	jit(&prog, &dfa, &ast, "\n");
	re_ast_deinit(&ast);
	deinit_dfa(&dfa);

	outf = fopen(out, "w");
	write_bin(outf, prog.code, prog.length);
	fclose(outf);
	a64_jit_destroy(&prog);
}

int main(void)
{
	test_jit("(hej)|(snopp)|[0-9]*", "files/jit1.bin");
	return 0;
}