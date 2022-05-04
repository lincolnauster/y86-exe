#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "memory.h"

// Let's allocate 4 MB of memory. Because virtual memory, this is all allocated
// lazily :)
#define MEMSIZE ((4096 * 1024))

struct memory {
	struct {
		uint64_t *x;
		int fd;
	} alloc;

	uint64_t prog_size;
};

struct memory *
mloadf(FILE *f)
{
	void *map;
	int zf;
	long len;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	if (len > MEMSIZE) {
		fprintf(stderr, "\033[1;31mGiven executable is too large.\033[0m\n");
		return NULL;
	}

	zf = open("/dev/zero", O_RDWR);
	if (zf < 0) {
		fprintf(stderr, "\033[1;31m/dev/zero somehow inaccessible.\033[0m\n");
		return NULL;
	}

	map = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, zf, 0);
	if (map == MAP_FAILED) {
		fprintf(stderr, "\033[1;31mCouldn't map memory for emulation.\033[0m\n");
		fprintf(stderr, "\Err #%d\n", errno);
		return NULL;
	}

	fread(map, 1, len, f);

	struct memory *m = malloc(sizeof(struct memory));
	if (!m) {
		munmap(map, MEMSIZE);
		return NULL;
	}

	m->alloc.x = map;
	m->alloc.fd = zf;
	m->prog_size = len;

	return m;
}

void
mdest(struct memory *m)
{
	munmap(m->alloc.x, MEMSIZE);
	close(m->alloc.fd);
	free(m);
}
