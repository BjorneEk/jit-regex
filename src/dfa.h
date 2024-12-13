#ifndef _DFA_H_
#define _DFA_H_

#include "regex.h"
#include "util/dla.h"
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

CODEGEN_DLA(u8_t, bytes)
void make_dfa(dfa_t *dfa, re_ast_t *ast, resz_t root);

void deinit_dfa(dfa_t *dfa);

void print_dfa_digraph(FILE *out, dfa_t *dfa, re_ast_t *ast);

void print_dfa(FILE *out, dfa_t *dfa, re_ast_t *ast);

bool is_accepting_state(re_ast_t *ast, state_t *s);

bool state_transition_inv(dfa_t *dfa, int idx, dla_t *dsts);
#endif /* _DFA_H_ */
