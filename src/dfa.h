#ifndef _DFA_H_
#define _DFA_H_

#include "regex.h"
#include <stdio.h>

typedef struct dfastate {
	resz_t *restates;
	resz_t length;
	resz_t transition[256];
} state_t;

typedef struct dfa {
	state_t *states;
	int length;
	int allocated;
} dfa_t;



void make_dfa(dfa_t *dfa, re_ast_t *ast, resz_t root);

void deinit_dfa(dfa_t *dfa);

void print_dfa_digraph(FILE *out, dfa_t *dfa, re_ast_t *ast);

void print_dfa(FILE *out, dfa_t *dfa, re_ast_t *ast);

#endif /* _DFA_H_ */
