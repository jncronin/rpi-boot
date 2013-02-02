#ifndef FS_H
#define FS_H

#include <dirent.h>
#include <stdio.h>
#include "block.h"

struct fs {
	struct block_device *parent;
	const char *fs_name;

	FILE *(*fopen)(struct fs *, struct dirent *, const char *mode);
	size_t (*fread)(struct fs *, void *ptr, size_t size, size_t nmemb, FILE *stream);
	int (*fclose)(struct fs *, FILE *fp);

	struct dirent *(*read_directory)(struct fs *, char **name);
};

#endif

