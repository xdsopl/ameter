/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#ifndef UTILS_H
#define UTILS_H
unsigned get_ticks();
char *string_time(char *fmt);
void readable_1024(WINDOW *pad, unsigned long long value);
void aligned_1024(WINDOW *pad, unsigned long long value);
#endif
