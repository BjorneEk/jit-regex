#ifndef _REGEX_H_
#define _REGEX_H_

#include "util/types.h"

typedef enum re_type {
	RE_NULL = 0,
	RE_ACCEPT,
	RE_CAT,
	RE_STAR,
	RE_OR,
	RE_CHAR
} re_type_t;

#define RE_INF_LEN (~0LLU)

typedef unsigned short resz_t;

typedef struct regex {
	u8_t ty;
	char ch;
	resz_t first, last;
	resz_t children[2];
} re_t;

typedef struct szbuff {
	resz_t *data;
	resz_t allocated;
	resz_t len;
} szbuff_t;

typedef struct re_ast {
	re_t *tree;
	szbuff_t *follows;
	resz_t allocated;
	resz_t length;
	resz_t *data;
	resz_t allocated_data;
	resz_t data_length;
	u64_t	min_length;
	u64_t	max_length;
} re_ast_t;

void re_ast_deinit(re_ast_t *ast);

void szbuff_init(szbuff_t *buff, resz_t init);

void szbuff_add(szbuff_t *buff, resz_t v);

resz_t re_add_data(re_ast_t *ast, resz_t *data, resz_t len);

resz_t *re_child(re_ast_t *ast, resz_t n, resz_t child);

szbuff_t *re_follows(re_ast_t *ast, resz_t n);

void re_setchar(re_ast_t *ast, resz_t n, char c);

re_type_t re_type(re_ast_t *ast, resz_t n);

char re_char(re_ast_t *ast, resz_t n);

void re_ast_init(re_ast_t *ast, resz_t init);

resz_t parse_regex(re_ast_t *ast, const char *regex);

void regex_print(re_ast_t *ast, resz_t root, bool first);

resz_t re_lastlen(re_ast_t *ast, resz_t n);
resz_t *re_last(re_ast_t *ast, resz_t n);

resz_t re_firstlen(re_ast_t *ast, resz_t n);
resz_t *re_first(re_ast_t *ast, resz_t n);

void re_init_follows(re_ast_t *ast);
void re_add_follows(re_ast_t *ast, resz_t n, resz_t v);
#endif
