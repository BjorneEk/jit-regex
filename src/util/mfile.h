/*==========================================================*
 *
 * @author Gustaf Franzén :: https://github.com/BjorneEk;
 *
 * mapped files
 *
 *==========================================================*/

#ifndef _MFILE_H_
#define _MFILE_H_

#include <stdlib.h>

typedef struct mfile {
	char *data;
	size_t size;
} mfile_t;

int mfile_open(mfile_t *file, const char *filename);

void mfile_close(mfile_t *file);

#endif /* _MFILE_H_ */