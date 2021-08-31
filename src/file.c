//SPDX-License-Identifier: GPL-2.0-or-later
/*
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file.h"

int file_len(const char *path, uint32_t *len)
{
	struct stat st;
	int ret;

	ret = stat(path, &st);
	if (ret) {
		printf("Failed to stat() file %s: %d\n", path, ret);
		return ret;
	}

	*len = st.st_size;

	return 0;
}

int file_open_and_mmap(const char *path, uint32_t len, struct mmapped_file *result, bool write)
{
	int fd, ret;
	void *map;

	assert(path);
	assert(len);
	assert(result);

	/* open the file */
	if (write)
		fd = open(path, O_TRUNC | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	else
		fd = open(path, O_RDONLY);

	if (fd < 0) {
		printf("Couldn't open file %s for %s.\n", path, write ? "writing" : "reading");
		return fd;
	}

	/* If writing make sure the file is the full size before mmap() */
	if (write) {
		/* make the file the full output size */
		ret = fallocate(fd, 0, 0, len);
		if (ret) {
			printf("fallocate() failed: %d\n", ret);
			return ret;
		}
	}

	/* mmap it */
	if (write)
		map = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	else
		map = mmap(0, len, PROT_READ, MAP_SHARED, fd, 0);

	if (!map) {
		printf("Failed to mmap file %s\n", path);
		return -ENOMEM;
	}

	if (write)
		memset(map, 0, len);

	result->fd = fd;
	result->map = map;
	result->len = len;

	return 0;
}

void file_unmap_and_close(struct mmapped_file *file)
{
	/* un-mmap */
	if (file->map)
		munmap(file->map, file->len);

	/* close file */
	if (file->fd >= 0)
		close(file->fd);
}
