#include <stdio.h>
#include "../src/regex.h"

int test_re_len(const char *re, u64_t min, u64_t max)
{
	re_ast_t ast;
	int res;
	re_ast_init(&ast, 30);
	parse_regex(&ast, re);
	res = 0;
	if (min != ast.min_length || max != ast.max_length) {
		fprintf(stderr, "unexpected size range for regex: '%s' (%llu, %llu) != (%llu, %llu)\n", re, min, max, ast.min_length, ast.max_length);
		res = 1;
	}

	re_ast_deinit(&ast);
	return res;
}

int main(void)
{
	int res;
	res = 0;
	res += test_re_len("(hej)|(snopp)", 3, 5);
	res += test_re_len("x?", 0, 1);
	res += test_re_len("(x*)|(a?)", 0, RE_INF_LEN);
	return res;
}