#ifndef DIRENT_H
#define DIRENT_H

struct dirent;
struct dir_info { 
	struct dirent *first;
	struct dirent *next;
};

#ifdef DIR
#undef DIR
#endif
#define DIR struct dir_info

#include "vfs.h"
#include <stdint.h>
#include "multiboot.h"




#endif

