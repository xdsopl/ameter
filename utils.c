/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <time.h>
#include <curses.h>

unsigned get_ticks()
{
	static struct timespec last;
	struct timespec current;
	clock_gettime(CLOCK_MONOTONIC, &current);
	unsigned diff = (current.tv_sec - last.tv_sec) * 1000
		+ (current.tv_nsec - last.tv_nsec) / 1000000;
	last = current;
	return diff ? diff : 1;
}

char *string_time(char *fmt)
{
	static char s[64];
	time_t now = time(0);
	strftime(s, sizeof(s), fmt, localtime(&now));
	return s;
}

void readable_1024(WINDOW *pad, unsigned long long value)
{
	char *prefix[] = { "", "ki", "mi", "gi", "ti", "pi" };
	unsigned i = 0;
	while (i < sizeof(prefix) / sizeof(*prefix)) {
		if (value < 10240)
			break;
		value = (value + 512) / 1024;
		i++;
	}
	wprintw(pad, "%llu%s", value, prefix[i]);
}

void aligned_1024(WINDOW *pad, unsigned long long value)
{
	char *prefix[] = { "", "ki", "mi", "gi", "ti", "pi" };
	unsigned i = 0;
	while (i < sizeof(prefix) / sizeof(*prefix)) {
		if (value < 10240)
			break;
		value = (value + 512) / 1024;
		i++;
	}
	if (!i)
		wprintw(pad, "  %5llu%s", value, prefix[i]);
	else
		wprintw(pad, "%5llu%s", value, prefix[i]);
}

int seperator(WINDOW *pad, int compact)
{
	if (compact)
		return 0;
	waddch(pad, '\n');
	return 1;
}

