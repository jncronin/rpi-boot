#ifndef FS_H
#define FS_H

#include <dirent.h>
#include <stdio.h>
#include "block.h"

struct fs {
	struct block_device *parent;
	const char *fs_name;

	FILE *(*fopen)(struct fs *, const char *path, const char *mode);
	size_t (*fread)(struct fs *, void *ptr, size_t size, size_t nmemb, FILE *stream);
	int (*fclose)(struct fs *, FILE *fp);

	DIR *(*opendir)(struct fs *, const char *name);
	struct dirent *(*readdir)(struct fs *, DIR *dirp);
	int (*closedir)(struct fs *, DIR *dirp);

	struct dirent *(*read_directory)(struct fs *, char **name);
};

#endif

