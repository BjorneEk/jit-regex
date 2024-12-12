
#include "mfile.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int mfile_open(mfile_t *file, const char *filename)
{
	int fd;
	struct stat s;

	fd = open(filename, O_RDONLY, S_IRUSR | S_IWUSR);

	if (fstat(fd, &s) == -1) {
		close(fd);
		return -1;
		//fprintf(stderr, "Unable to read size of file %s\n", filename);
		//exit(-1);
	}
	file->size = s.st_size;
	file->data = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	close(fd);
	return 0;
}

void mfile_close(mfile_t *file)
{
	munmap(file->data, file->size);
}