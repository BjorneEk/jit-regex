/*==========================================================*
 *
 * @author Gustaf Franzén :: https://github.com/BjorneEk;
 *
 * jit types
 *
 *==========================================================*/

#ifndef _A64_JIT_H_
#define _A64_JIT_H_

#include "a64_ins.h"
#include <stdlib.h>

#define A64_JIT_ERROR "A64 JIT Error"
typedef struct a64_jit {
	a64_t		*code;
	size_t		length;
	size_t		real_size;
	bool		executable;
} a64_jit_t;


void a64_jit_init(a64_jit_t *jit, size_t length);
void a64_jit_mkexec(a64_jit_t *jit);
void a64_jit_destroy(a64_jit_t *jit);

void a64_jit_push(a64_jit_t *jit, a64_t ins);
size_t a64_jit_push_data(a64_jit_t *jit, void *data, size_t length);

#endif /* _A64_JIT_H_ */