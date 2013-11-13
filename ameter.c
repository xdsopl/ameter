/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include "utils.h"

int handle_cpu_stat(WINDOW *pad, unsigned ticks, int compact);
int handle_mem_info(WINDOW *pad);
int handle_net_stat(WINDOW *pad, unsigned ticks);
int handle_disk_stat(WINDOW *pad, unsigned ticks);

int main()
{
	WINDOW *stdscr = initscr();
	WINDOW *pad = newpad(getmaxy(stdscr), getmaxx(stdscr) + 1);
	int compact = 0;
	while (1) {
		wclear(pad);
		wresize(pad, getmaxy(stdscr), getmaxx(stdscr) + 1);
		int rows = 1;
		unsigned ticks = get_ticks();
		waddstr(pad, string_time("%F %T\n"));
		rows += seperator(pad, compact);
		rows += handle_cpu_stat(pad, ticks, compact);
		rows += seperator(pad, compact);
		rows += handle_mem_info(pad);
		rows += seperator(pad, compact);
		rows += handle_net_stat(pad, ticks);
		rows += seperator(pad, compact);
		rows += handle_disk_stat(pad, ticks);
		if (rows > getmaxy(stdscr))
			compact = 1;
		prefresh(pad, 0, 0, 0, 0, getmaxy(stdscr) - 1, getmaxx(stdscr) - 1);
		sleep(3);
	}
	return 0;
}

