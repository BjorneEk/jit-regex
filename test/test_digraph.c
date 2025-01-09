#include <stdio.h>
#include "../src/regex.h"
#include "../src/dfa.h"

void test_digraph(const char *re, const char *out)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;
	FILE *outf;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root, DFA_OPT_FULL);

	outf = fopen(out, "w");
	print_dfa_digraph(outf, &dfa, &ast);
	fclose(outf);

	re_ast_deinit(&ast);
	deinit_dfa(&dfa);
}

int main(void)
{
	test_digraph("(hej)|(abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ)|[0-9]*", "files/digraph.dot");
	return 0;
}