/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "utils.h"

struct mem_info {
	unsigned long total, free, buffers, cached, swap_total, swap_free;
};

static void update_mem_info(struct mem_info *mem)
{
	FILE *proc_meminfo = fopen("/proc/meminfo", "r");
	char str[4096];
	while (fgets(str, sizeof(str), proc_meminfo)) {
		if (!strncmp(str, "MemTotal:", strlen("MemTotal:")))
			sscanf(str, "MemTotal:%lu kB", &mem->total);
		else if (!strncmp(str, "MemFree:", strlen("MemFree:")))
			sscanf(str, "MemFree:%lu kB", &mem->free);
		else if (!strncmp(str, "Buffers:", strlen("Buffers:")))
			sscanf(str, "Buffers:%lu kB", &mem->buffers);
		else if (!strncmp(str, "Cached:", strlen("Cached:")))
			sscanf(str, "Cached:%lu kB", &mem->cached);
		else if (!strncmp(str, "SwapTotal:", strlen("SwapTotal:")))
			sscanf(str, "SwapTotal:%lu kB", &mem->swap_total);
		else if (!strncmp(str, "SwapFree:", strlen("SwapFree:")))
			sscanf(str, "SwapFree:%lu kB", &mem->swap_free);
	}
	fclose(proc_meminfo);
}

static int show_mem_info(WINDOW *pad, struct mem_info *mem)
{
	unsigned long used = mem->total - mem->free - mem->buffers - mem->cached;
	int width = (getmaxx(pad)-1) - 5 * 10 - 7;
	width = width < 0 ? 0 : width;
	int used_width = (width * used + mem->total / 2) / mem->total;
	int buffers_width = (width * mem->buffers + mem->total / 2) / mem->total;
	int cached_width = (width * mem->cached + mem->total / 2) / mem->total;
	int free_width = width - used_width - buffers_width - cached_width;
	waddstr(pad, "mem: [");
	for (int i = 0; i < used_width; i++)
		waddch(pad, 'u');
	for (int i = 0; i < buffers_width; i++)
		waddch(pad, 'b');
	for (int i = 0; i < cached_width; i++)
		waddch(pad, 'c');
	for (int i = 0; i < free_width; i++)
		waddch(pad, ' ');
	waddstr(pad, "] t=");
	readable_1024(pad, 1024 * mem->total);
	waddstr(pad, "b u=");
	readable_1024(pad, 1024 * used);
	waddstr(pad, "b b=");
	readable_1024(pad, 1024 * mem->buffers);
	waddstr(pad, "b c=");
	readable_1024(pad, 1024 * mem->cached);
	waddstr(pad, "b f=");
	readable_1024(pad, 1024 * mem->free);
	waddstr(pad, "b\n");
	return 1;
}

int handle_mem_info(WINDOW *pad)
{
	struct mem_info mem;
	update_mem_info(&mem);
	return show_mem_info(pad, &mem);
}

