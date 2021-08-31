//SPDX-License-Identifier: GPL-2.0-or-later
/*
 */
#ifndef __FILE_H__
#define __FILE_H__

#include <stdbool.h>
#include <stdint.h>

struct mmapped_file {
	int fd;
	void *map;
	uint32_t len;
};

int file_len(const char *path, uint32_t *len);
int file_open_and_mmap(const char *path, uint32_t len, struct mmapped_file *result, bool write);
void file_unmap_and_close(struct mmapped_file *file);

#endif /* __FILE_H__ */
