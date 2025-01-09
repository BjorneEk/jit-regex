#ifndef _JIT_H_
#define _JIT_H_
#include "../include/a64_ins.h"
#include "../include/a64_jit.h"
#include "dfa.h"
#include "util/dla.h"
#include <stdio.h>

typedef enum branch_type {
	STATE,
	END,
	START,
	REDO,
	REDO_INNER,
	MISS,
	CMP
} branch_type_t;

typedef struct branch {
	branch_type_t	ty;
	a64_cond_t		cond;
	u64_t			instruction_idx;
	u32_t			destination_block_index;
} branch_t;
typedef enum data_ptr_type {
	STATE_PTR,
	DATA_PTR
} data_ptr_type_t;
typedef struct data_ptr {
	data_ptr_type_t ty;
	u64_t instruction_idx;
	u32_t idx;
} data_ptr_t;

typedef struct block {
	u64_t offset;
	dla_t instructions;
	dla_t branches;
	dla_t data_ptrs;
	u32_t idx;
} block_t;

typedef struct data_block {
	u64_t offset;
	u64_t length;
	u8_t *data;
} data_block_t;

typedef struct mach {
	dla_t blocks;
	dla_t data_blocks;
	u32_t start;
	u32_t end;
	u32_t miss;
	union {
		struct {
			u32_t redo;
			u32_t redo_inner;
		};
		struct {
			u32_t mcmp;
		};
	};
	u32_t length;
} mach_t;

void jit(a64_jit_t *prog, dfa_t *dfa, re_ast_t *ast, const char *endchars);
void jit_count_matches(a64_jit_t *prog, dfa_t *dfa, re_ast_t *ast);
#endif /* _JIT_H_ */