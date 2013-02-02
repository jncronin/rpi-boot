#include "vfs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static struct vfs_entry *first = (void*)0;
static struct vfs_entry *def = (void*)0;

static void vfs_add(struct vfs_entry *ve)
{
	ve->next = first;
	first = ve;
}

static struct vfs_entry *find_ve(const char *path)
{
	struct vfs_entry *cur = first;
	while(cur)
	{
		if(!strcmp(cur->device_name, path))
			return cur;
		cur = cur->next;
	}
	return (void *)0;
}

static void free_split_dir(char **sp)
{
	char **s = sp;
	while(*s)
	{
		free(*s);
		s++;
	}
	free(sp);
}

static void free_dirent_list(struct dirent *d)
{
	while(d)
	{
		struct dirent *tmp = d;
		d = d->next;
		free(tmp);
	}
}

static char **split_dir(const char *path, struct vfs_entry **ve)
{
	int dir_start = 0;
	int dir_count = 0;

	// First iterate through counting the number of directories
	int slen = strlen(path);
	int reading_dev = 0;

	*ve = def;

	for(int i = 0; i < slen; i++)
	{
		if(path[i] == '(')
		{
			if(i == 0)
				reading_dev = 1;
			else
			{
				printf("VFS: dir parse error, invalid '(' in %s position %i\n",
						path, i);
				return (void*)0;
			}
		}
		else if(path[i] == ')')
		{
			if(!reading_dev)
			{
				printf("VFS: dir parse error, invalid ')' in %s position %i\n",
						path, i);
				return (void*)0;
			}
			// The device name runs from position 1 to 'i'
			char *dev_name = (char *)malloc(i);
			strncpy(dev_name, &path[1], i - 1);
			dev_name[i - 1] = 0;
			*ve = find_ve(dev_name);
			reading_dev = 0;
			dir_start = i + 1;
		}
		else if(path[i] == '/')
		{
			if(reading_dev)
			{
				printf("VFS: dir parse error, invalid '/' in device name in %s "
						"at position %i\n", path, i);
				return (void*)0;
			}

			if(i != (dir_start))
				dir_count++;
			else
				dir_start++;
		}
		else if(i == (slen - 1))
			dir_count++;
	}

	if(*ve == (void*)0)
	{
		printf("VFS: unable to determine device name when parsing %s\n", path);
		return (void*)0;
	}

	// Now iterate through again assigning to the path array
	int cur_dir = 0;
	char **ret = (char **)malloc((dir_count + 1) * sizeof(char *));
	ret[dir_count] = 0;	// null terminate
	int cur_idx = dir_start;
	int cur_dir_start = dir_start;
	while(cur_dir < dir_count)
	{
		while(path[cur_idx] != '/')
			cur_idx++;
		// Found a '/'
		int path_bit_length = cur_idx - cur_dir_start;
		char *pb = (char *)malloc((path_bit_length + 1) * sizeof(char));
		for(int i = 0; i < path_bit_length; i++)
			pb[i] = path[cur_dir_start + i];
		pb[path_bit_length] = 0;

		cur_idx++;
		cur_dir_start = cur_idx;
		ret[cur_dir++] = pb;
	}

	return ret;
}

int vfs_register(struct fs *fs)
{
	if(fs == (void *)0)
		return -1;
	if(fs->parent == (void *)0)
		return -1;

	struct vfs_entry *ve = (struct vfs_entry *)malloc(sizeof(struct vfs_entry));
	memset(ve, 0, sizeof(struct vfs_entry));

	ve->device_name = fs->parent->device_name;
	ve->fs = fs;
	vfs_add(ve);

	if(def == (void*)0)
		def = ve;

	return 0;
}

void vfs_list_devices()
{
	struct vfs_entry *cur = first;
	while(cur)
	{
		printf("%s ", cur->device_name);
		cur = cur->next;
	}
}

struct dirent *read_directory(const char *path)
{
	char **p;
	struct vfs_entry *ve;
	p = split_dir(path, &ve);
	if(p == (void *)0)
		return (void *)0;

	struct dirent *ret = ve->fs->read_directory(ve->fs, p);
	free_split_dir(p);
	return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if(stream == (void *)0)
		return 0;

	size_t bytes_to_read = size * nmemb;
	if(bytes_to_read > (size_t)(stream->len - stream->pos))
		bytes_to_read = (size_t)(stream->len - stream->pos);
	size_t nmemb_to_read = bytes_to_read / size;
	bytes_to_read = nmemb_to_read * size;

	bytes_to_read = stream->fs->fread(stream->fs, ptr, 1, bytes_to_read, stream);
	return bytes_to_read / size;
}

FILE *fopen(const char *path, const char *mode)
{
	char **p;
	struct vfs_entry *ve;
	p = split_dir(path, &ve);
	if(p == (void *)0)
		return (void *)0;

	// Trim off the last entry
	char **p_iter = p;
	while(*p_iter)
		p_iter++;
	char *fname = p[(p_iter - p) - 1];
	p[(p_iter - p) - 1] = 0;

	// Read the containing directory
	struct dirent *dir = ve->fs->read_directory(ve->fs, p);
	struct dirent *dir_start = dir;
	if(dir == (void*)0)
	{
		free_split_dir(p);
		free(fname);
		return (void*)0;
	}

	struct dirent *file = (void *)0;
	while(dir)
	{
		if(!strcmp(dir->name, fname))
		{
			file = dir;
			break;
		}
		dir = dir->next;
	}

	free_split_dir(p);
	free(fname);

	if(!file)
	{
		free_dirent_list(dir_start);
		return (void*)0;
	}

	// Read the file
	FILE *ret = ve->fs->fopen(ve->fs, file, mode);
	free_dirent_list(dir_start);
	return ret;
}

