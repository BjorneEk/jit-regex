#include "regex.h"
#include "util/dla.h"
#include "dfa.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <unistd.h>

static bool nullable(re_ast_t *ast, resz_t n)
{
	switch (re_type(ast, n)) {
		case RE_OR: return nullable(ast, *re_child(ast, n, 0)) || nullable(ast, *re_child(ast, n, 1));
		case RE_CAT: return nullable(ast, *re_child(ast, n, 0)) && nullable(ast, *re_child(ast, n, 1));
		case RE_STAR: return true;
		case RE_CHAR: return false;
		case RE_ACCEPT: return false;
		case RE_NULL: return true;
		default:
			return false;
	}
}

static void union_(resz_t *res, resz_t *len_res, const resz_t *a, resz_t len_a, const resz_t *b, resz_t len_b)
{
	int i = 0;
	int j = 0;
	int k = 0;

	while (i < len_a && j < len_b) {
		if (a[i] < b[j]) {
			res[k++] = a[i++];
		} else if (a[i] > b[j]) {
			res[k++] = b[j++];
		} else {
			res[k++] = a[i++];
			j++;
		}
	}

	if (i < len_a) {
		memmove(&res[k], &a[i], (len_a - i) * sizeof(resz_t));
		k += len_a - i;
	}

	if (j < len_b) {
		memmove(&res[k], &b[j], (len_b - j) * sizeof(resz_t));
		k += len_b - j;
	}

	*len_res = k;
}

static void setfirst(re_ast_t *ast, resz_t n)
{
	resz_t _first[256];
	resz_t *first = &_first[1];

	switch (re_type(ast, n)) {
		case RE_OR:
			setfirst(ast, *re_child(ast, n, 0));
			setfirst(ast, *re_child(ast, n, 1));
			union_(
				first,
				_first,
				re_first(ast, *re_child(ast, n, 0)),
				re_firstlen(ast, *re_child(ast, n, 0)),
				re_first(ast, *re_child(ast, n, 1)),
				re_firstlen(ast, *re_child(ast, n, 1)));
			break;
		case RE_CAT:
			setfirst(ast, *re_child(ast, n, 0));
			setfirst(ast, *re_child(ast, n, 1));
			if (nullable(ast, *re_child(ast, n, 0))) {
				union_(
					first,
					_first,
					re_first(ast, *re_child(ast, n, 0)),
					re_firstlen(ast, *re_child(ast, n, 0)),
					re_first(ast, *re_child(ast, n, 1)),
					re_firstlen(ast, *re_child(ast, n, 1)));
			} else {
				_first[0] = re_firstlen(ast, *re_child(ast, n, 0));
				memmove(first, re_first(ast, *re_child(ast, n, 0)), _first[0] * sizeof(resz_t));
			}
			break;
		case RE_STAR:
			setfirst(ast, *re_child(ast, n, 0));
			_first[0] = re_firstlen(ast, *re_child(ast, n, 0));
			memmove(first, re_first(ast, *re_child(ast, n, 0)), _first[0] * sizeof(resz_t));
			break;
		case RE_ACCEPT:
		case RE_CHAR:
			_first[0] = 1;
			first[0] = n;
			break;
		case RE_NULL:
			_first[0] = 0;
			break;
	}
	ast->tree[n].first = re_add_data(ast, _first, _first[0]);
}

static void setlast(re_ast_t *ast, resz_t n)
{
	resz_t _last[256];
	resz_t *last = &_last[1];

	switch (re_type(ast, n)) {
		case RE_OR:
			setlast(ast, *re_child(ast, n, 0));
			setlast(ast, *re_child(ast, n, 1));
			union_(
				last,
				_last,
				re_last(ast, *re_child(ast, n, 0)),
				re_lastlen(ast, *re_child(ast, n, 0)),
				re_last(ast, *re_child(ast, n, 1)),
				re_lastlen(ast, *re_child(ast, n, 1)));
			break;
		case RE_CAT:
			setlast(ast, *re_child(ast, n, 0));
			setlast(ast, *re_child(ast, n, 1));
			if (nullable(ast, *re_child(ast, n, 1))) {
				union_(
					last,
					_last,
					re_last(ast, *re_child(ast, n, 0)),
					re_lastlen(ast, *re_child(ast, n, 0)),
					re_last(ast, *re_child(ast, n, 1)),
					re_lastlen(ast, *re_child(ast, n, 1)));
			} else {
				_last[0] = re_lastlen(ast, *re_child(ast, n, 1));
				memmove(last, re_last(ast, *re_child(ast, n, 1)), _last[0] * sizeof(resz_t));
			}
			break;
		case RE_STAR:
			setlast(ast, *re_child(ast, n, 0));
			_last[0] = re_lastlen(ast, *re_child(ast, n, 0));
			memmove(last, re_last(ast, *re_child(ast, n, 0)), _last[0] * sizeof(resz_t));
			break;
		case RE_ACCEPT:
		case RE_CHAR:
			_last[0] = 1;
			last[0] = n;
			break;
		case RE_NULL:
			_last[0] = 0;
			break;
	}
	ast->tree[n].last = re_add_data(ast, _last, _last[0]);
}

static void _setfollows(re_ast_t *ast, resz_t root)
{
	int i, j;
	resz_t *l1, *l2;
	switch (re_type(ast, root)) {
		case RE_CAT:
			l1 = re_last(ast, *re_child(ast, root, 0));
			l2 = re_first(ast, *re_child(ast, root, 1));

			for (i = 0; i < re_lastlen(ast, *re_child(ast, root, 0)); i++) {
				for (j = 0; j < re_firstlen(ast, *re_child(ast, root, 1)); j++) {
					re_add_follows(ast, l1[i], l2[j]);
				}
			}
			_setfollows(ast, *re_child(ast, root, 0));
			_setfollows(ast, *re_child(ast, root, 1));
			break;
		case RE_OR:
			_setfollows(ast, *re_child(ast, root, 0));
			_setfollows(ast, *re_child(ast, root, 1));
			break;
		case RE_STAR:
			l1 = re_last(ast, *re_child(ast, root, 0));
			l2 = re_first(ast, *re_child(ast, root, 0));

			for (i = 0; i < re_lastlen(ast, *re_child(ast, root, 0)); i++) {
				for (j = 0; j < re_firstlen(ast, *re_child(ast, root, 0)); j++) {
					re_add_follows(ast, l1[i], l2[j]);
				}
			}
			_setfollows(ast, *re_child(ast, root, 0));
			//_setfollows(ast, *re_child(ast, root, 1));
			break;
		default:break;
	}
}

static void setfollows(re_ast_t *ast, resz_t root)
{
	re_init_follows(ast);
	_setfollows(ast, root);
}

void dfa_init(dfa_t *dfa, int init)
{
	dfa->allocated = init;
	dfa->length = 0;
	dfa->states = malloc(init * sizeof(state_t));
}

static int dfa_newstate(dfa_t *dfa)
{
	int res;

	if (dfa->length + 1 >= dfa->allocated) {
		dfa->allocated += 3;
		dfa->states = realloc(dfa->states, dfa->allocated * sizeof(state_t));
	}
	res = dfa->length;
	++dfa->length;
	return res;
}

static void get_sc(re_ast_t *ast, state_t *S, char c, resz_t *sc, int *sc_len)
{
	int i, len;
	resz_t n;

	len = 0;
	for (i = 0; i < S->length; i++) {
		n = S->restates[i];
		if (re_type(ast, n) == RE_CHAR && re_char(ast, n) == c) {
			sc[len++] = n;
		}
	}
	*sc_len = len;
}

static void insert_sorted_unique(resz_t *arr, int *len, resz_t val)
{
	int pos = 0;

	while (pos < *len && arr[pos] < val) {
		pos++;
	}

	if (pos < *len && arr[pos] == val)
		return;

	memmove(&arr[pos + 1], &arr[pos], (*len - pos) * sizeof(resz_t));

	arr[pos] = val;

	(*len)++;
}
static void get_follows(re_ast_t *ast, resz_t *follows, int *follows_len, resz_t *sc, int sc_len)
{
	int i, j;
	szbuff_t *f;

	for (i = 0; i < sc_len; i++) {
		f = re_follows(ast, sc[i]);
		for (j = 0; j < f->len; j++)
			insert_sorted_unique(follows, follows_len, f->data[j]);
	}
}

static void parr(FILE *out, resz_t *arr, resz_t n);
static int get_state(dfa_t *dfa, resz_t *re, int re_len)
{
	int i;

	for (i = 0; i < dfa->length; i++) {
		if (re_len == dfa->states[i].length &&
			memcmp(re, dfa->states[i].restates, re_len * sizeof(resz_t)) == 0) {
			return i + 1;
		}
	}
	return 0;
}

static void dfagen(dfa_t *dfa, re_ast_t *ast, int state)
{
	int i;
	state_t *S_;
	resz_t sc[256], follows[256];
	int sc_len, follows_len;
	int s_, S_idx;


	for (i = 0; i < 256; i++) {
		sc_len = 0;
		follows_len = 0;
		get_sc(ast, &dfa->states[state], i, sc, &sc_len);
		get_follows(ast, follows, &follows_len, sc, sc_len);
		s_ = get_state(dfa, follows, follows_len);
		if (s_ == 0 && follows_len > 0) {
			S_idx = dfa_newstate(dfa);
			S_ = &dfa->states[S_idx];
			S_->length = follows_len;
			S_->restates = calloc(follows_len, sizeof(resz_t));
			memmove(S_->restates, follows, follows_len * sizeof(resz_t));
			memset(S_->transition, 0, sizeof(S_->transition));
			dfa->states[state].transition[i] = S_idx + 1;
			dfagen(dfa, ast, S_idx);
		} else {
			dfa->states[state].transition[i] = follows_len > 0 ? s_ : 0;
		}
	}
}

void make_dfa(dfa_t *dfa, re_ast_t *ast, resz_t root)
{
	resz_t S0_len;
	state_t *S0;
	int S0_idx;

	setfirst(ast, root);
	setlast(ast, root);
	setfollows(ast, root);
	dfa_init(dfa, 5);

	S0_idx = dfa_newstate(dfa);
	S0 = &dfa->states[S0_idx];

	S0_len = re_firstlen(ast, root);
	S0->restates = calloc(S0_len, sizeof(resz_t));
	memmove(S0->restates, re_first(ast, root), S0_len * sizeof(resz_t));
	S0->length = S0_len;
	memset(S0->transition, 0, sizeof(S0->transition));

	dfagen(dfa, ast, S0_idx);
}

void deinit_dfa(dfa_t *dfa)
{
	int i;

	for (i = 0; i < dfa->length; i++)
		free(dfa->states[i].restates);
	free(dfa->states);
}

bool is_accepting_state(re_ast_t *ast, state_t *s)
{
	int i;

	for (i = 0; i < s->length; i++) {
		if (re_type(ast, s->restates[i]) == RE_ACCEPT)
			return true;
	}
	return false;
}

static void parr(FILE *out, resz_t *arr, resz_t n)
{
	int i;
	fprintf(out, "{");
	for (i = 0; i < n - 1; i++)
		fprintf(out, "%d, ", arr[i]);
	fprintf(out, "%d}", arr[n - 1]);
}

static void print_state(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	state_t *s;


	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	if (idx == 0)
		fprintf(out, "Start S1: ");
	else
		fprintf(out, "S%d: ", idx + 1);
	parr(out, s->restates, s->length);
	fprintf(out, " %c", accepting ? '[' : '{');

	for (i = 0; i < 256; i++) {
		if (s->transition[i] != 0) {
			if (isprint((unsigned char)i)) {
				fprintf(out, "(%c -> S%d)", i, s->transition[i]);
			} else {
				fprintf(out, "(0x%X -> S%d)", i, s->transition[i]);
			}
		}
	}
	fprintf(out, "%c\n", accepting ? ']' : '}');
}

static void print_state_dot(FILE *out, dfa_t *dfa, re_ast_t *ast, int idx)
{
	bool accepting;
	int i;
	state_t *s;


	s = &dfa->states[idx];
	accepting = is_accepting_state(ast, s);

	for (i = 0; i < 256; i++) {
		if (s->transition[i] != 0) {
			if (isprint((unsigned char)i)) {
				fprintf(out, "	S%d -> S%d [label=\"%c\"];\n", idx + 1, s->transition[i], i);
			} else {
				fprintf(out, "	S%d -> S%d [label=\"0x%X\"];\n", idx + 1, s->transition[i], i);
			}
		}
	}
	if (accepting)
		fprintf(out, "S%d [shape=doublecircle];\n", idx + 1);
}

void print_dfa(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i;
	for (i = 0; i < dfa->length; i++) {
		print_state(out, dfa, ast, i);
	}
}

void print_dfa_digraph(FILE *out, dfa_t *dfa, re_ast_t *ast)
{
	int i;
	printf("generated digraph, view digraph on 'https://dreampuf.github.io/GraphvizOnline'\n");
	fprintf(out, "digraph StateMachine {\n	start -> S1\n");
	for (i = 0; i < dfa->length; i++) {
		print_state_dot(out, dfa, ast, i);
	}
	fprintf(out, "}");
}

//CODEGEN_DLA(u8_t, bytes)

bool state_transition_inv(dfa_t *dfa, int idx, dla_t *dsts)
{
	int i;
	bool used;
	state_t *s;

	s = &dfa->states[idx];
	for (i = 0; i < dfa->length; i++)
		bytes_init(&dsts[i], 10);
	used = 0;
	for (i = 0; i < 256; i++)
		if (s->transition[i] != 0) {
			used = true;
			bytes_append(&dsts[s->transition[i] - 1], (u8_t)i);
		}
	return used;
}