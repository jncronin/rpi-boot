/* Copyright (C) 2017 by Alexander Graf <agraf@suse.de>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfdt.h>
#include "uart.h"
#include "atag.h"
#include "fb.h"
#include "console.h"
#include "block.h"
#include "vfs.h"
#include "memchunk.h"
#include "usb.h"
#include "dwc_usb.h"
#include "output.h"
#include "log.h"
#include "rpifdt.h"

#include "config_parse.h"

static int method_kernel(char *args);
static int method_arm_control(char *args);

#ifdef __aarch64__
const char *boot_kernel = "kernel8.img";
#else
const char *boot_kernel = "kernel7.img";
#endif
uint32_t boot_arm_control;

static const struct config_parse_method methods[] =
{
	{
		.name = "kernel",
		.method = method_kernel,
	}, {
		.name = "arm_control",
		.method = method_arm_control,
	}, {
		.name = NULL,
	},
};

static int method_kernel(char *args)
{
	boot_kernel = args;

	return 0;
}

static int method_arm_control(char *args)
{
	char *endptr;
	boot_arm_control = strtol(args, &endptr, 0);

	return 0;
}

void find_and_run_config(void)
{
	char **dev = vfs_get_device_list();
	FILE *f = NULL;
	const char *fname = "/config.txt";
	int fname_len = strlen(fname);
	const char *found_cfg = NULL;
	void (*entry)(long a, long b, long c);
	char *kernel_fname;

	/* The default load address for payloads is 0x80000 */
	entry = (void*)0x80000;

	/* Look for config.txt */
	for (dev = vfs_get_device_list(); *dev; dev++) {
		int dev_len = strlen(*dev);
		char *new_str = (char *)malloc(dev_len + fname_len + 3);

		sprintf(new_str, "(%s)%s", *dev, fname);
		f = fopen(new_str, "r");

		if (f) {
			found_cfg = new_str;
			break;
		}

		free(new_str);
	}

	if (f) {
		long flen = fsize(f);
		char *buf = (char *)malloc(flen+1);

		printf("MAIN: Found bootloader configuration: %s\n", found_cfg);

		/* Read full file */
		buf[flen] = 0;
		fread(buf, 1, flen, f);
		fclose(f);

		config_parse(buf, '=', methods);
	} else {
		printf("MAIN: No config.txt file found, using defaults\n");
	}

	if (*dev) {
		kernel_fname = malloc(strlen(*dev) + strlen("/") + strlen(boot_kernel) + 1);
		sprintf(kernel_fname, "(%s)/%s", *dev, boot_kernel);
	} else {
		kernel_fname = malloc(strlen("/") + strlen(boot_kernel) + 1);
		sprintf(kernel_fname, "/%s", boot_kernel);
	}

	f = fopen(kernel_fname, "r");
	if (!f) {
		printf("Could not open kernel \"%s\". Aborting\n", boot_kernel);
		return;
	}

#ifdef __aarch64__
	if (!(boot_arm_control & 0x200)) {
		printf("Sorry, I can not run 32bit kernels. Aborting\n");
		return;
	}
#endif

	fread((void*)entry, 1, fsize(f), f);
	fclose(f);

	printf("Calling kernel ...\n");
	entry(0, 0, 0);
}
