#ifndef _SHM_UTILS_H_
#define _SHM_UTILS_H_

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/threads.h>
#include <sys/msg.h>
#include <trace.h>
#include <sys/mman.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>


static inline char *shm_init(char *path, bool creat, int bufsz)
{
	int ret;
	int flags = O_RDWR;

	if (creat) {
		flags |= O_CREAT;
	}

	int fd = open(path, flags);
	if (fd < 0) {
		fprintf(stderr, "open failed: %d\n", errno);
		return NULL;
	}

	ret = ftruncate(fd, bufsz);
	if (ret < 0) {
		fprintf(stderr, "ftruncate failed: %d\n", fd);
		return NULL;
	}

	struct stat s;
	ret = fstat(fd, &s);
	if (ret < 0) {
		fprintf(stderr, "%d fail: %d\n", __LINE__, ret);
		return NULL;
	}

	size_t fileSize = s.st_size;
	// fprintf(stderr, "size=%zu\n", fileSize);

	assert(fileSize == bufsz);

	// fprintf(stderr, "fd=%d\n", fd);
	char *v = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (v == MAP_FAILED) {
		fprintf(stderr, "mmap failed\n");
		return NULL;
	}

	return v;
}

#endif
