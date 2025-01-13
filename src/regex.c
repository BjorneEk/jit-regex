
#include "regex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


void szbuff_init(szbuff_t *buff, resz_t init)
{
	buff->allocated = init;
	buff->data = malloc(init * sizeof(resz_t));
	buff->len = 0;
}

void szbuff_add(szbuff_t *buff, resz_t v)
{
	if (buff->len + 1 >= buff->allocated) {
		buff->allocated = MAX(buff->allocated * 2, buff->allocated + 5);
		buff->data = realloc(buff->data, buff->allocated * sizeof(resz_t));
	}
	buff->data[buff->len++] = v;
}

void re_add_follows(re_ast_t *ast, resz_t n, resz_t v)
{
	int i;

	i = 0;
	while (i < ast->follows[n].len && ast->follows[n].data[i] < v)
		++i;
	szbuff_add(&ast->follows[n], v);
	memmove(ast->follows[n].data + i + 1, ast->follows[n].data + i,
	(ast->follows[n].len - 1 - i) * sizeof(resz_t));
	ast->follows[n].data[i] = v;
}

szbuff_t *re_follows(re_ast_t *ast, resz_t n)
{
	return &ast->follows[n];
}

void re_init_follows(re_ast_t *ast)
{
	int i;

	ast->follows = malloc(ast->length * sizeof(szbuff_t));

	for (i = 0; i < ast->length; i++)
		szbuff_init(&ast->follows[i], 5);
}

void re_ast_init(re_ast_t *ast, resz_t init)
{
	ast->tree = malloc(init * sizeof(re_t));
	ast->allocated = init;
	ast->follows = NULL;
	ast->length = 0;
	ast->data = malloc(init * sizeof(resz_t));
	ast->allocated_data = init;
	ast->data_length = 0;
}

resz_t re_add_data(re_ast_t *ast, resz_t *data, resz_t len)
{
	resz_t res;
	if (ast->data_length + len + 1 >= ast->allocated_data) {
		ast->allocated_data = MAX(ast->allocated_data * 2, ast->allocated_data + 2 * len);
		ast->data = realloc(ast->data, ast->allocated_data * sizeof(resz_t));
	}
	memmove(ast->data + ast->data_length, data, (len + 1) * sizeof(resz_t));
	res = ast->data_length;
	ast->data_length += len + 1;
	return res;
}

resz_t re_firstlen(re_ast_t *ast, resz_t n)
{
	return ast->data[ast->tree[n].first];
}
resz_t *re_first(re_ast_t *ast, resz_t n)
{
	return &ast->data[ast->tree[n].first + 1];
}
resz_t re_lastlen(re_ast_t *ast, resz_t n)
{
	return ast->data[ast->tree[n].last];
}
resz_t *re_last(re_ast_t *ast, resz_t n)
{
	return &ast->data[ast->tree[n].last + 1];
}


static resz_t re_add(re_ast_t *ast, re_type_t ty)
{
	resz_t res;

	if (1 + ast->length >= ast->allocated) {
		ast->allocated = MAX(10, ast->allocated * 2);
		ast->tree = realloc(ast->tree, ast->allocated * sizeof(re_t));
	}
	ast->tree[ast->length].ty = ty;
	res = ast->length;
	++ast->length;

	return res;
}

resz_t *re_child(re_ast_t *ast, resz_t n, resz_t child)
{
	return &ast->tree[n].children[child];
}

void re_setchar(re_ast_t *ast, resz_t n, char c)
{
	ast->tree[n].ch = c;
}

re_type_t re_type(re_ast_t *ast, resz_t n)
{
	return ast->tree[n].ty;
}

char re_char(re_ast_t *ast, resz_t n)
{
	return ast->tree[n].ch;
}

typedef struct pctx {
	const char *re;
	int i;
} pctx_t;

static int parse_char(pctx_t *ctx)
{
	return ctx->re[ctx->i] == '\\' ? (++ctx->i, ctx->re[ctx->i++]) : ctx->re[ctx->i++];
}

static resz_t re_parse_range_inner(re_ast_t *ast, pctx_t *ctx)
{
	int c1, c2;
	resz_t next, child1, child2;

	c1 = parse_char(ctx);

	if (ctx->re[ctx->i] != '-') {
		fprintf(stderr, "Error parsing regex, expected '-' found '%c'\n", ctx->re[ctx->i]);
		exit(-1);
	}
	++ctx->i;
	c2 = parse_char(ctx);

	if (c2 < c1) {
		fprintf(stderr, "Error parsing regex, invalid range [%d-%d]\n", c1, c2);
		exit(-1);
	}


	child1 = re_add(ast, RE_CHAR);
	re_setchar(ast, child1, c1);
	++c1;

	for(;;) {
		if (c1 > c2) {
			return child1;
		}
		child2 = re_add(ast, RE_CHAR);
		re_setchar(ast, child2, c1);
		next = re_add(ast, RE_OR);
		*re_child(ast, next, 1) = child1;
		*re_child(ast, next, 0) = child2;
		child1 = next;
		++c1;
	}
}

static resz_t re_parse_range(re_ast_t *ast, pctx_t *ctx)
{
	resz_t next, child1, child2;

	child1 = re_parse_range_inner(ast, ctx);

	for(;;) {
		if (ctx->re[ctx->i] == ']') {
			++ctx->i;
			return child1;
		}
		child2 = re_parse_range_inner(ast, ctx);
		next = re_add(ast, RE_OR);
		*re_child(ast, next, 0) = child1;
		*re_child(ast, next, 1) = child2;
		child1 = next;
	}
}
static resz_t re_parse(re_ast_t *ast, resz_t prev, pctx_t *ctx);
static resz_t re_parse_tl(re_ast_t *ast, pctx_t *ctx)
{
	int c;
	resz_t node, child;
	c = ctx->re[ctx->i++];

	switch (c) {
		case '[':
			node = re_parse_range(ast, ctx);
			return re_parse(ast, node, ctx);
		case '(':
			child = re_parse_tl(ast, ctx);
			node = re_parse(ast, child, ctx);
			return node;
		case '\0':
			fprintf(stderr, "Error parsing regex\n");
			return 0;
		case '\\':
			c = ctx->re[ctx->i++];
		default:
			node = re_add(ast, RE_CHAR);
			re_setchar(ast, node, c);
			return re_parse(ast, node, ctx);
	}
}

static resz_t re_parse(re_ast_t *ast, resz_t prev, pctx_t *ctx)
{
	int c;
	resz_t node, child, child_;
	c = ctx->re[ctx->i++];

	switch (c) {
		case '+':
			node = re_add(ast, RE_CAT);
			*re_child(ast, node, 0) = prev;
			child = re_add(ast, RE_STAR);
			*re_child(ast, child, 0) = prev;
			*re_child(ast, node, 1) = child;
			return re_parse(ast, node, ctx);
		case '*':
			node = re_add(ast, RE_STAR);
			*re_child(ast, node, 0) = prev;
			return re_parse(ast, node, ctx);
		case '|':
			node = re_add(ast, RE_OR);
			*re_child(ast, node, 0) = prev;
			child = re_parse_tl(ast, ctx);
			*re_child(ast, node, 1) = child;
			return node;
		case '?':
			node = re_add(ast, RE_OR);
			*re_child(ast, node, 0) = prev;
			*re_child(ast, node, 1) = re_add(ast, RE_NULL);
			return re_parse(ast, node, ctx);
		case '(':
			node = re_add(ast, RE_CAT);
			child = re_parse_tl(ast, ctx);
			child_ = re_parse(ast, child, ctx);
			*re_child(ast, node, 0) = prev;
			*re_child(ast, node, 1) = child_;
			return node;
		case '\0':
		case ')':
			return prev;
		case '[':
			child = re_parse_range(ast, ctx);
			child_ = re_parse(ast, child, ctx);
			node = re_add(ast, RE_CAT);
			*re_child(ast, node, 0) = prev;
			*re_child(ast, node, 1) = child_;
			return node;
		case '\\':
			c = ctx->re[ctx->i++];
		default:
			child = re_add(ast, RE_CHAR);
			/* printf("char: '%c'\n", c); */
			re_setchar(ast, child, c);
			child_ = re_parse(ast, child, ctx);
			/* printf("child, n: %d %d\n", child, node); */
			node = re_add(ast, RE_CAT);
			*re_child(ast, node, 0) = prev;
			*re_child(ast, node, 1) = child_;
			return node;
	}
}

static void calc_max_len(re_ast_t *ast, resz_t node, u64_t *max)
{
	u64_t m1, m2;
	if (*max == RE_INF_LEN)
		return;

	switch (re_type(ast, node)) {
		case RE_NULL:
		case RE_ACCEPT:
			break;
		case RE_CAT:
			calc_max_len(ast, *re_child(ast, node, 0), max);
			calc_max_len(ast, *re_child(ast, node, 1), max);
			break;
		case RE_STAR:
			*max = RE_INF_LEN;
			break;
		case RE_OR:
			m1 = m2 = 0;
			calc_max_len(ast, *re_child(ast, node, 0), &m1);
			calc_max_len(ast, *re_child(ast, node, 1), &m2);
			*max += m1 >= m2 ? m1 : m2;
			break;
		case RE_CHAR:
			++*max;
			break;
	}
}

static void calc_min_len(re_ast_t *ast, resz_t node, u64_t *min)
{
	u64_t m1, m2;
	if (*min == RE_INF_LEN)
		return;

	switch (re_type(ast, node)) {
		case RE_NULL:
		case RE_ACCEPT:
		case RE_STAR:
			break;
		case RE_CAT:
			calc_min_len(ast, *re_child(ast, node, 0), min);
			calc_min_len(ast, *re_child(ast, node, 1), min);
			break;
		case RE_OR:
			m1 = m2 = 0;
			calc_min_len(ast, *re_child(ast, node, 0), &m1);
			calc_min_len(ast, *re_child(ast, node, 1), &m2);
			*min += m1 <= m2 ? m1 : m2;
			break;
		case RE_CHAR:
			++*min;
			break;
	}
}

resz_t parse_regex(re_ast_t *ast, const char *regex)
{
	pctx_t ctx;
	resz_t root;
	resz_t acc, cat;
	u64_t min, max;
	ctx.re = regex;
	ctx.i = 0;
	root = re_parse_tl(ast, &ctx);
	cat = re_add(ast, RE_CAT);
	acc = re_add(ast, RE_ACCEPT);
	*re_child(ast, cat, 0) = root;
	*re_child(ast, cat, 1) = acc;
	min = max = 0;
	calc_min_len(ast, cat, &min);
	calc_max_len(ast, cat, &max);
	ast->max_length = max;
	ast->min_length = min;
	return cat;
}

void re_ast_deinit(re_ast_t *ast)
{
	int i;
	free(ast->tree);
	if (ast->follows != NULL) {
		for (i = 0; i < ast->length; i++)
			free(ast->follows[i].data);
		free(ast->follows);
	}
	free(ast->data);
}

static void parr(FILE *out, resz_t *arr, resz_t n)
{
	int i;
	fprintf(out, "{");
	for (i = 0; i < n - 1; i++)
		fprintf(out, "%d, ", arr[i]);
	fprintf(out, "%d}", arr[n - 1]);
}

static void reast_print(FILE *out, re_ast_t *ast, resz_t node, char *pre, bool final, bool first)
{
	char buff[4096];

	fprintf(out, "%s", pre);
	strcpy(buff, pre);
	if (final) {
		strcat(buff, "    ");
		fprintf(out, "└───");
	} else {
		strcat(buff, "│   ");
		fprintf(out, "├───");
	}

	switch (re_type(ast, node)) {
		case RE_OR:
			fprintf(out, "┬ or");
			if (first) {
				parr(out, re_first(ast, node), re_firstlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_last(ast, node), re_lastlen(ast, node));
			}
			fprintf(out, "\n");
			reast_print(out, ast, *re_child(ast, node, 0), buff, false, first);
			reast_print(out, ast, *re_child(ast, node, 1), buff, true, first);
			break;
		case RE_CAT:
			fprintf(out, "┬ cat");
			if (first) {
				parr(out, re_first(ast, node), re_firstlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_last(ast, node), re_lastlen(ast, node));

			}
			fprintf(out, "\n");

			reast_print(out, ast, *re_child(ast, node, 0), buff, false, first);
			reast_print(out, ast, *re_child(ast, node, 1), buff, true, first);
			break;
		case RE_CHAR:
			fprintf(out, "─ '%c'", re_char(ast, node));
			if (first) {
				parr(out, re_first(ast, node), re_firstlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_last(ast, node), re_lastlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_follows(ast, node)->data, re_follows(ast, node)->len);
			}
			fprintf(out, "\n");
			break;
		case RE_ACCEPT:
			fprintf(out, "─ ACCEPT");
			if (first) {
				parr(out, re_first(ast, node), re_firstlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_last(ast, node), re_lastlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_follows(ast, node)->data, re_follows(ast, node)->len);
			}
			fprintf(out, "\n");
			break;
		case RE_NULL:
			fprintf(out, "─ '{}'");
			if (first) {
				parr(out, re_first(ast, node), re_firstlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_last(ast, node), re_lastlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_follows(ast, node)->data, re_follows(ast, node)->len);
			}
			fprintf(out, "\n");
			break;
		case RE_STAR:
			fprintf(out, "┬ *");
			if (first) {
				parr(out, re_first(ast, node), re_firstlen(ast, node));
				fprintf(out, ", ");
				parr(out, re_last(ast, node), re_lastlen(ast, node));

			}
			fprintf(out, "\n");

			reast_print(out, ast, *re_child(ast, node, 0), buff, true, first);
			break;
		default:
			fprintf(out, "ERROR");
			break;
	}
}

void regex_print(re_ast_t *ast, resz_t root, bool first)
{
	reast_print(stdout, ast, root, "", true, first);
}


