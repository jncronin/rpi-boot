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

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "stdio.h"

const char lowercase[] = "0123456789abcdef";
const char uppercase[] = "0123456789abcdef";

int (*stderr_putc)(int c);
int (*stdout_putc)(int c);
int (*stream_putc)(int c, FILE *stream);

int fputc(int c, FILE *stream)
{
	if(stream == stdout)
		return stdout_putc(c);
	else if(stream == stderr)
		return stderr_putc(c);
	else
		return stream_putc(c, stream);
}

int putc(int c, FILE *stream)
{
	return fputc(c, stream);
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int fputs(const char *s, FILE *stream)
{
	while(*s)
		fputc(*s++, stream);
	return 0;
}

int puts(const char *s)
{
	fputs(s, stdout);
	fputc('\n', stdout);
	return 0;
}

void puthex(uint32_t val)
{
	for(int i = 7; i >= 0; i--)
		putchar(lowercase[(val >> (i * 4)) & 0xf]);
}

void putval(uint32_t val, int base, char *dest, int dest_size, int dest_start, int padding, char *case_str)
{
	int i;

	if(padding > (dest_size - dest_start))
		padding = dest_size - dest_start;

	for(i = 0; i < padding; i++)
		dest[i + dest_start] = '0';

	i = 0;
	while((val != 0) && (i < (dest_size - dest_start)))
	{
		uint32_t digit = val % base;
		dest[dest_size - i - 1 + dest_start] = case_str[digit];
		i++;
		val /= base;
	}
}

#define __O_WRITE(c) { s[o] = c; if(++o >= (int)n) return o; }

#if 0
int vsnprintf(char *s, size_t n, const char *format, va_list arg)
{
	int reading_escape = 0;
	int reading_field = 0;
	int escape_start = -1;
	int field_start = -1;

	int i = 0;
	int o = 0;

	while(format[i])
	{
		if(reading_escape)
		{
			switch(format[i])
			{
				case '\\':
				case 'n':
				case 't':
					__O_WRITE('\\');
					__O_WRITE(format[i]);
					i++;
					reading_escape = 0;
					continue;
			}

		}
		else
		{
			if(format[i] == '%')
			{
				int alt_form = 0;
				int zero_pad = 0;
				int left_adj = 0;
				int leave_space = 0;
				int always_sign = 0;
				
				reading_field = 1;
				field_start = i;

				i++;

				if(format[i] == '#')
				{
					alt_form = 1;
					i++;
				}
				if(format[i] == '0')
				{
					zero_pad = 1;
					i++;
				}
				if(format[i] == '-')
				{
					left_adj = 1;
					i++;
				}
				if(format[i] == ' ')
				{
					leave_space = 1;
					i++;
				}
				if(format[i] == '+')
				{
					always_sign = 1;
					i++;
				}

				// Read field width
				int field_width_start = i;
				while((format[i] >= '0') && (format[i] <= '9'))
					i++;
				int field_width_end = i;

				int field_width = -1;

				if(field_width_start != field_width_end)
				{
					int field_width_base = 1;
					field_width = 0;

					for(int j = field_width_end - 1; j >= field_width_start; j++)
					{
						field_width += field_width_base * (format[j] - '0');
						field_width_base *= 10;
					}
				}

				// Read precision
				int precision = -1;
				if(format[i] == '.')
				{
					int precision_negative = 0;
					i++;
					if(format[i] == '-')
					{
						precision_negative = 1;
						i++;
					}
					int precision_start = i;
					while((format[i] >= '0') && (format[i] <= '9'))
						i++;
					int precision_end = i;

					if(precision_start != precision_end)
					{
						int precision_base = 1;
						precision = 0;

						for(int j = precision_end - 1; j >= precision_start; j++)
						{
							precision += precision_base * (format[j] - '0');
							precision_base *= 10;
						}

						if(precision_negative)
							precision = -precision;
					}
				}

				// Read length modifier
				int length = 0;

#define __L_HH	1
#define __L_H	2
#define __L_L	3
#define __L_LL	4
#define __L_LD	5
#define __L_J	6
#define __L_Z	7
#define __L_T	8

				if(format[i] == 'h')
				{
					i++;
					if(format[i] == 'h')
					{
						length = __L_HH;
						i++;
					}
					else
						length = __L_H;
				}
				else if(format[i] == 'l')
				{
					i++;
					if(format[i] == 'l')
					{
						length = __L_LL;
						i++;
					}
					else
						length = __L_L;
				}
				else if(format[i] == 'L')
				{
					i++;
					length = __L_LD;
				}
				else if(format[i] == 'q')
				{
					i++;
					length = __L_LL;
				}
				else if(format[i] == 'j')
				{
					i++;
					length = __L_J;
				}
				else if(format[i] == 'z')
				{
					i++;
					length = __L_Z;
				}
				else if(format[i] == 't')
				{
					i++;
					length = __L_T;
				}

				// read conversion specifier
				char conv = format[i];
				i++;

				
				
				continue;
			}
			else if(format[i] == '\\')
			{
				reading_escape = 1;
				escape_start = i;
				i++;
				continue;
			}
			else
			{
				__O_WRITE(format[i]);
				i++;
				continue;
			}
		}
	}

	return o;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
	
}

#endif

