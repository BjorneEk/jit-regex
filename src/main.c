#include <stdio.h>
#include "dfa.h"
#include "regex.h"
#include "jit/aarch64_jit.h"
#include "util/mfile.h"

int (*lex)(const char**,const char**);

void grep(const char *fname)
{
	const char *b, *e, *ls;
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
		strncpy(buff, ls, e - ls);
		buff[e - ls] = '\0';
		puts(buff);
		ls = b = e + 1;
		goto loop;
	}
	for(;;) {
		if (*b == '\n')
			break;
		if (*b == '\0')
			return;
		++b;
	}
	++b;
	ls = b;
	goto loop;
}

#ifndef DEBUG_MAIN
int main(int argc, char *argv[])
{
	re_ast_t ast;
	aarch64_prog_t prog;
	resz_t root;
	dfa_t dfa;
	int i;
	if (argc < 2) {
		printf("Unexpected arguments\n%s	<regex> <file(s)>\n", argv[0]);
		exit(-1);
	}

	re_ast_init(&ast, 30);
	root = parse_regex(&ast, argv[1]);
	make_dfa(&dfa, &ast, root);
	aarch64_jit(&prog, &dfa, &ast, "\n");
	re_ast_deinit(&ast);
	deinit_dfa(&dfa);


	lex = (int (*)(const char**,const char**))(uintptr_t)prog.code;

	for (i = 2; i < argc; i++)
		grep(argv[i]);

	aarch64_prog_deinit(&prog);
	return 0;
}
#endif
