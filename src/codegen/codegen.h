#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include <stdio.h>
#include "../dfa.h"

void codegen_c(FILE *out, dfa_t *dfa, re_ast_t *ast);
#endif /* _CODEGEN_H_ */