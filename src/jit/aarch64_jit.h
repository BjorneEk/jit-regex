#ifndef _AARCH64_JIT_H_
#define _AARCH64_JIT_H_
#include "../aarch64/aarch64_ins.h"
#include "../dfa.h"
#include "../util/dla.h"
#include <stdio.h>

typedef enum aarch64_branch_type {
	STATE,
	END,
	START,
	REDO,
	REDO_INNER,
	MISS
} aarch64_branch_type_t;

typedef struct aarch64_branch {
	aarch64_branch_type_t	ty;
	aarch64_cond_t		cond;
	u64_t			instruction_idx;
	u32_t			destination_block_index;
} aarch64_branch_t;

typedef struct aarch64_block {
	u64_t offset;
	dla_t instructions;
	dla_t branches;
	u32_t idx;
} aarch64_block_t;

typedef struct aarch64_mach {
	dla_t blocks;
	u32_t start;
	u32_t end;
	u32_t redo;
	u32_t redo_inner;
	u32_t miss;
	u32_t length;
} aarch64_mach_t;

typedef struct aarch64_prog {
	aarch64_t	*code;
	size_t		size;
} aarch64_prog_t;

void aarch64_prog_deinit(aarch64_prog_t *prog);
void aarch64_jit(aarch64_prog_t *prog, dfa_t *dfa, re_ast_t *ast, const char *endchars);
void aarch64_write_bin(FILE *fp, const aarch64_prog_t *prog);
#endif /* _AARCH65_JIT_H_ */