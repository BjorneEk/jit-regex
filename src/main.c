#include <stdio.h>
#include "dfa.h"
#include "regex.h"
#include "jit/aarch64_jit.h"
#include "util/mfile.h"
#include "util/types.h"
#include <pthread.h>

typedef struct part {
	char *start;
	size_t length;
	unsigned long cnt;
} part_t;

typedef struct pfile {
	mfile_t file;
	dla_t parts;
} pfile_t;

CODEGEN_DLA(part_t, parts)

int (*lex)(const char**,const char**);

#define MIN_PART (100)
#define NTHREADS (16)
bool split_part(pfile_t *f, u32_t pidx)
{
	u32_t pos;
	part_t *p;

	p = parts_getptr(&f->parts, pidx);
	pos = p->length / 2;

	if (pos < MIN_PART)
		return false;

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
void partition_file(const char *fname, pfile_t *f)
{

	mfile_open(&f->file, fname);

	parts_init(&f->parts, 16);

	parts_append(&f->parts, (part_t){
		.start = f->file.data,
		.length = f->file.size
	});

	split_part(f, 0);
	split_part(f, 0);
	split_part(f, 1);

	split_part(f, 0);
	split_part(f, 1);
	split_part(f, 2);
	split_part(f, 3);

	split_part(f, 0);
	split_part(f, 1);
	split_part(f, 2);
	split_part(f, 3);
	split_part(f, 4);
	split_part(f, 5);
	split_part(f, 6);
	split_part(f, 7);
}

void *grepcnt_part(void *arg)
{
	part_t *p =arg;
	const char *b, *e;
	b = p->start;
	p->cnt = 0;
loop:
	if (lex(&b, &e)) {

		for(;;) {
			if (*e == '\n')
				break;
			if (*e == '\0' || e - p->start >= p->length)
				return NULL;
			++e;
		}
		++p->cnt;
		b = e + 1;
		goto loop;
	}
	for(;;) {
		if (*b == '\n')
			break;
		if (*b == '\0' || b - p->start >= p->length)
			return NULL;
		++b;
	}
	++b;
	goto loop;
}
void pgcount(const char *fname)
{
	pfile_t f;
	pthread_t threads[NTHREADS - 1];
	part_t *p;
	int i;
	unsigned long res;
	partition_file(fname, &f);
	p = parts_getptr(&f.parts, 0);
	for (i = 0; i < f.parts.length - 1; i++)
		pthread_create(&threads[i], NULL, grepcnt_part, parts_getptr(&f.parts, i+1));
	grepcnt_part(p);
	res = p->cnt;
	for (i = 0; i < f.parts.length - 1; i++) {
		pthread_join(threads[i], NULL);
		res += parts_getptr(&f.parts, i+1)->cnt;
	}
	printf("%lu\n", res);
	parts_deinit(&f.parts);
	mfile_close(&f.file);

}
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
void grepcnt(const char *fname)
{
	const char *b, *e;
	mfile_t file;
	unsigned long cnt = 0;
	mfile_open(&file, fname);
	b = file.data;
loop:
	if (lex(&b, &e)) {
		for(;;) {
			if (*e == '\n')
				break;
			if (*e == '\0') {
				printf("%lu\n", cnt);
				return;
			}
			++e;
		}
		++cnt;
		b = e + 1;
		goto loop;
	}
	for(;;) {
		if (*b == '\n')
			break;
		if (*b == '\0') {
			printf("%lu\n", cnt);
			return;
		}
		++b;
	}
	++b;
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
	int i0 = 2;
	void (*func)(const char *) = grep;
	if (argc < 2) {
		printf("Unexpected arguments\n%s	<regex> <file(s)>\n", argv[0]);
		exit(-1);
	}

	re_ast_init(&ast, 30);
	if (!strcmp(argv[1], "-c")) {
		func = pgcount;
		i0 = 3;
	}
	root = parse_regex(&ast, argv[i0 - 1]);
	make_dfa(&dfa, &ast, root);
	aarch64_jit(&prog, &dfa, &ast, "\n");
	re_ast_deinit(&ast);
	deinit_dfa(&dfa);


	lex = (int (*)(const char**,const char**))(uintptr_t)prog.code;

	for (i = i0; i < argc; i++)
		func(argv[i]);

	aarch64_prog_deinit(&prog);
	return 0;
}
#endif
