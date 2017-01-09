/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Parses the .cfg file

#include "config_parse.h"
#include "ctype.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static char *read_line(char **buf)
{
	char *start = *buf;
	char *ptr = *buf;

	if(!*start)
		return (void*)0;
	while((*ptr != 0) && (*ptr != '\n'))
		ptr++;

	if(*ptr == 0)
	{
		// End of the string
		*buf = ptr;
		return start;
	}
	else
	{
		// End of a line - null terminate
		*ptr = 0;
		ptr++;
		*buf = ptr;
		return start;
	}
}

/* Remove leading and trailing whitespace and trailing comments from a configuration line */
static char *strip_comments(char *buf)
{
	// Remove leading whitespace
	while(isspace(*buf))
		buf++;

	// Remove comments
	char *s = buf;
	while(*s)
	{
		if(*s == '#')
			*s = '\0';
		else
			s++;
	}

	// Remove trailing whitespace
	s--;
	while((s > buf) && isspace(*s))
	{
		*s = '\0';
		s--;
	}

	return buf;
}

const char empty_string[] = "";

void split_string(char *str, char **method, char **args)
{
	int state = 0;
	char *p = str;

	// state = 	0 - reading spaces before method
	// 		1 - reading method
	// 		2 - reading spaces before argument

	*method = (char*)empty_string;
	*args = (char*)empty_string;

	while(*p)
	{
		if(*p == ' ')
		{
			if(state == 1)
			{
				*p = 0;	// null terminate method
				state = 2;
			}
		}
		else
		{
			if(state == 0)
			{
				*method = p;
				state = 1;
			}
			else if(state == 2)
			{
				*args = p;
				return;
			}
		}
		p++;
	}
}

int config_parse(char *buf, const struct config_parse_method *methods)
{
	char *line;
	char *b = buf;
	const struct config_parse_method *curmethod;

	while((line = read_line(&b)))
	{
		line = strip_comments(line);
#ifdef MULTIBOOT_DEBUG
		printf("read_line: %s\n", line);
#endif
		char *method, *args;
		split_string(line, &method, &args);
#ifdef MULTIBOOT_DEBUG
		printf("method: %s, args: %s\n", method, args);
#endif

		if(!strcmp(method, empty_string))
			continue;

		// Find and run the method
		int found = 0;
		char *lwr = strlwr(method);

		for(curmethod = methods; curmethod->name; curmethod++)
		{
			if(!strcmp(lwr, curmethod->name))
			{
				found = 1;
				int retno = curmethod->method(args);
				if(retno != 0)
				{
					printf("cfg_parse: %s failed with "
							"%i\n", line,
							retno);
					free(lwr);
					return retno;
				}
				break;
			}
		}

		free(lwr);

		if(!found)
			printf("cfg_parse: unknown method %s\n", method);
	}

	return 0;
}

