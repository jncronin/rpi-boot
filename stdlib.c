#include <string.h>
#include <stddef.h>
#include <stdio.h>

int errno;

void *memcpy(void *dest, const void *src, size_t n)
{
	char *s = (char *)src;
	char *d = (char *)dest;
	while(n > 0)
	{
		*d++ = *s++;
		n--;
	}
	return dest;
}

void *memset(void *s, int c, size_t n)
{
	char *dest = (char *)s;
	while(n > 0)
	{
		*dest++ = (char)c;
		n--;
	}
	return s;
}

void abort(void)
{
	fputs("abort() called", stdout);
	fputs("abort() called", stderr);

	while(1);
}

char *strcpy(char *dest, const char *src)
{
	char *d = dest;
	while(*src != 0)
		*d++ = *src++;
	*d = 0;
	return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	char *d = dest;
	while((*src != 0) && (n > 0))
	{
		*d++ = *src++;
		n--;
	}
	if(n > 0)
		*d = 0;
	return dest;
}

size_t strlen(const char *s)
{
	size_t ret = 0;
	while(*s++ != 0) ret++;
	return ret;
}

int strcmp(const char *s1, const char *s2)
{
	while(*s1 || *s2)
	{
		char s = *s1++ - *s2++;
		if(s != 0)
			return (int)s;
	}
	return 0;
}


