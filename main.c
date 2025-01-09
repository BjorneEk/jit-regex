#include <stdio.h>
#include "include/a64_jit.h"
#include "src/regex.h"
#include "src/dfa.h"
#include "src/jit.h"
#include "src/util/mfile.h"



u64_t count(const char *re, const char *in)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;
	a64_jit_t prog;
	mfile_t f;
	u64_t res;
	u64_t (*jit_func)(const char *input);

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root, DFA_OPT_FULL);
	//regex_print(&ast, root, true);
	//codegen_c(stdout, &dfa, &ast, "\n");
	//print_dfa(stdout, &dfa, &ast);
	jit_func = (u64_t (*)(const char *input))jit_count_matches(&prog, &dfa, &ast);
	re_ast_deinit(&ast);
	deinit_dfa(&dfa);

	FILE *out = fopen("files/output.bin", "w");
	fwrite(prog.code, sizeof(a64_t), prog.length, out);
	fclose(out);
	a64_jit_mkexec(&prog);
	mfile_open(&f, in);
	res = jit_func(f.data);
	a64_jit_destroy(&prog);
	mfile_close(&f);
	return res;
}

int main(int argc, char *argv[])
{

	printf("%llu\n", count(argv[1], argv[2]));
	return 0;
}
