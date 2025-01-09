#include "test.h"
#
void write_bin(FILE *fp, a64_t *prog, size_t len)
{
	fwrite(prog, sizeof(a64_t), len, fp);
}