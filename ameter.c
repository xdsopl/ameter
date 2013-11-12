/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <unistd.h>
#include "utils.h"

int handle_cpu_stat(int term_width, int compact);
int handle_mem_info(int term_width);
int handle_net_stat(unsigned ticks);
int handle_disk_stat(unsigned ticks);

static int seperator(int compact)
{
	if (compact)
		return 0;
	fputc('\n', stderr);
	return 1;
}

int main()
{
	int compact = 0;
	while (1) {
		fprintf(stderr, "\E[H\E[2J");
		int term_width = 80;
		int term_height = 23;
		int rows = 1;
		fputs(string_time("%F %T\n"), stderr);
		rows += seperator(compact);
		rows += handle_cpu_stat(term_width, compact);
		rows += seperator(compact);
		rows += handle_mem_info(term_width);
		rows += seperator(compact);
		unsigned ticks = get_ticks();
		rows += handle_net_stat(ticks);
		rows += seperator(compact);
		rows += handle_disk_stat(ticks);
		if (rows > term_height)
			compact = 1;
		sleep(3);
	}
	return 0;
}

