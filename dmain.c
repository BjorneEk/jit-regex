#include <stdio.h>
#include "src/regex.h"
#include "src/dfa.h"
#include "src/codegen/codegen.h"
#include "src/util/dla.h"
#include "src/aarch64/aarch64_ins.h"
const char *float_re = "[0-9]+(.[0-9]*)?";


const char *c_float_re = "(([0-9]+.[0-9]*)|(.[0-9]+))(f|F)?";
const char *c_int_re = "(-[0-9]+)|([1-9][0-9]*)|(0((x[0-9a-fA-F]+)|([0-7])))";
const char *re_print = "print";

void test_regex(const char *re)
{
	re_ast_t ast;
	resz_t root;

	re_ast_init(&ast, 10);
	root = parse_regex(&ast, re);

	regex_print(&ast, root, false);

	re_ast_deinit(&ast);
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
void jit_test(char *bin)
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

	test_dfa(c_float_re, "files/float.dot", "files/float.c", "files/float.S");
	test_dfa(c_int_re,"files/int.dot", "files/int.c", "files/int.S");
	test_dfa(re_print,"files/print.dot", "files/print.c", "files/print.S");
	jit_test("files/jit.bin");
	return 0;
}
