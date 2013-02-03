#ifndef VFS_H
#define VFS_H

struct vfs_file;

#include "dirent.h"
#include "multiboot.h"

#ifdef FILE
#undef FILE
#define FILE struct vfs_file
#endif

#include "fs.h"

struct vfs_entry
{
	char *device_name;
	struct fs *fs;
	struct vfs_entry *next;
};

struct vfs_file
{
	struct fs *fs;
	long pos;
	void *opaque;
	long len;
};

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);

int vfs_register(struct fs *fs);
void vfs_list_devices();
char **vfs_get_device_list();
int vfs_set_default(char *dev_name);
char *vfs_get_default();

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif

