#include <stdio.h>
#include "src/regex.h"
#include "src/dfa.h"
#include "src/codegen/codegen.h"
#include "src/util/dla.h"
#include "src/jit/aarch64_ins.h"
const char *float_re = "[0-9]+(.[0-9]*)?";


const char *c_float_re = "(([0-9]+.[0-9]*)|(.[0-9]+))(f|F)?";
const char *c_int_re = "(-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7])))";


void test_regex(const char *re)
{
	re_ast_t ast;
	resz_t root;

	re_ast_init(&ast, 10);
	root = parse_regex(&ast, re);

	regex_print(&ast, root, false);

	re_ast_deinit(&ast);
}



void test_dfa(const char *re, char *dotname, char *cname)
{
	re_ast_t ast;
	resz_t root;
	dfa_t dfa;
	FILE *dot, *c;

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, re);

	make_dfa(&dfa, &ast, root);

	print_dfa(stdout, &dfa, &ast);

	dot = fopen(dotname, "w");
	c = fopen(cname, "w");

	print_dfa_digraph(dot, &dfa, &ast);

	codegen_c(c, &dfa, &ast);

	fclose(dot);
	fclose(c);

	re_ast_deinit(&ast);
	deinit_dfa(&dfa);
}

CODEGEN_DLA(aarch64_t, arm)
void jit_test(char *bin)
{
	dla_t code;
	FILE *fp;
	arm_init(&code, 10);

	arm_append(&code, aarch64_ldr(LDRX, R0, R1));
	arm_append(&code, aarch64_ldr(LDRB, R4, R6));
	arm_append(&code, aarch64_ldr(LDRSH, R20, R8));
	arm_append(&code, aarch64_str(STRX, R0, R1));
	arm_append(&code, aarch64_cond_branch(NE, -4));
	arm_append(&code, aarch64_add_imm(R1, R1, 1));

	fp = fopen(bin, "w");

	fwrite(code.data, sizeof(aarch64_t), code.length, fp);
	fclose(fp);
	arm_deinit(&code);

}

int main(void)
{
	//test_regex(c_float_re);
	//test_regex(c_int_re);

	test_dfa(c_float_re, "files/float.dot", "files/float.c");
	test_dfa(c_int_re,"files/int.dot", "files/int.c");
	jit_test("files/jit.bin");
	return 0;
}
