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

