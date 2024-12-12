#include <stdio.h>
#include "src/regex.h"
#include "src/dfa.h"

const char *float_re = "[0-9]+(.[0-9]*)?";


const char *c_float_re = "(([0-9]+.[0-9]*)|(.[0-9]+))(f|F)?";
const char *c_int_re = "(-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F])|([0-7])))";


void test_regex(const char *re)
{
	re_ast_t ast;
	resz_t root;

	re_ast_init(&ast, 10);
	root = parse_regex(&ast, re);

	regex_print(&ast, root, false);

	re_ast_deinit(&ast);
}



void test_dfa(const char *re, char *generated_dighraph_filename)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;
	FILE *fp;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root);

	print_dfa(stdout, &dfa, &ast);

	fp = fopen(generated_dighraph_filename, "w");
	print_dfa_digraph(fp, &dfa, &ast);
	fclose(fp);

	re_ast_deinit(&ast);
	deinit_dfa(&dfa);
}

int main(void)
{
	//test_regex(c_float_re);
	//test_regex(c_int_re);

	test_dfa(c_float_re, "files/float.dot");
	//test_dfa(c_int_re,"files/int.dot");
	return 0;
}
