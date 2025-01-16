#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_
#include "../include/a64_ins.h"
#include "../include/a64_jit.h"
#include "util/dla.h"
#include "util/types.h"

typedef enum instruction_dependency_type {
	BRANCH_TO_STATE,
	BRANCH_TO_LBL,
	LOAD_STATE_ADDRESS,
	LOAD_LBL_ADDRESS,
} dep_type_t;

typedef enum block_type {
	SBLOCK,
	IBLOCK,
	DBLOCK,
} block_type_t;

typedef struct instruction_dependency {
	dep_type_t ty;
	u64_t instruction_index;
	u32_t target_ident;
	union { a64_cond_t cond; a64_reg_t dst_reg; };
} idep_t;

typedef struct block {
	block_type_t ty;
	u32_t ident;
	u64_t offset;
	union {
		struct {dla_t instructions; dla_t deps;};
		struct {u64_t length; u8_t *data;};
	};
} block_t;

#endif