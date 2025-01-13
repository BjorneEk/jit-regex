#include <stdio.h>
#include "../src/regex.h"
#include "../src/dfa.h"

void test_dfa(const char *re)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root, DFA_OPT_FULL);
	//regex_print(&ast, root, true);
	//codegen_c(stdout, &dfa, &ast, "\n");
	print_dfa(stdout, &dfa, &ast);

	re_ast_deinit(&ast);
	deinit_dfa(&dfa);
}

int main(void)
{
	test_dfa("(hej)|(snopp)|[0-9]*");
	puts("");
	test_dfa("hej");
	puts("");
	test_dfa("h");
	return 0;
}