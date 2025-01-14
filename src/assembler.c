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
	IBLOCK,
	DBLOCK,
} block_type_t;

typedef struct instruction_dependency {
	dep_type_t ty;
	u64_t index;
	union { a64_cond_t cond; a64_reg_t dst_reg; };
	union { u32_t dst_state_idx; char *lbl; };
} idep_t;

typedef struct block {
	block_type_t ty;
	char *lbl;
	u64_t offset;
	u32_t state_idx;
	union {
		struct {dla_t instructions; dla_t deps;};
		struct {u64_t length; u8_t *data;};
	};
} block_t;

CODEGEN_DLA(idep_t, deps)
CODEGEN_DLA(block_t, blocks)

typedef struct assembly {
	dla_t blocks;
	u32_t length;
} assm_t;