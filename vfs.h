#ifndef VFS_H
#define VFS_H

struct vfs_file;
typedef struct vfs_file FILE;

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
int vfs_set_default(char *dev_name);
char *vfs_get_default();

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);
struct dirent *read_directory(const char *path);

#define SEEK_SET	0x1000
#define SEEK_CUR	0x1001
#define SEEK_END	0x1002
#define SEEK_START	SEEK_SET

#endif

