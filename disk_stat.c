/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

#define DISK_STAT_NUM_MAX (64)

struct disk_stat {
	char name[16];
	unsigned long rx, wx;
};

void parse_disk_stat(struct disk_stat *dev, char *str)
{
	str += 13;
	char *end = strchr(str, ' ');
	int len = end - str;
	if (!end || !len || len >= 16)
		return;
	if (!strncmp(str, "sd", 2) && isdigit(str[len-1]))
		return;
	if (!strncmp(str, "hd", 2) && isdigit(str[len-1]))
		return;
	memcpy(dev->name, str, len);
	dev->name[len] = 0;
	unsigned long tmp[5];
	sscanf(end, "%lu %lu %lu %lu %lu %lu %lu",
		&tmp[0], &tmp[1], &dev->rx, &tmp[2],
		&tmp[3], &tmp[4], &dev->wx);
}

void update_disk_stat(struct disk_stat *devs)
{
	memset(devs, 0, sizeof(struct disk_stat) * DISK_STAT_NUM_MAX);
	FILE *proc_diskstats = fopen("/proc/diskstats", "r");
	char str[4096];
	int i = 0;
	while (i < DISK_STAT_NUM_MAX && fgets(str, sizeof(str), proc_diskstats))
		if (strlen(str) > 13)
			parse_disk_stat(devs + i++, str);
	fclose(proc_diskstats);
}

void copy_disk_stat(struct disk_stat *dst, struct disk_stat *src)
{
	memcpy(dst, src, sizeof(struct disk_stat) * DISK_STAT_NUM_MAX);
}

int show_disk_stat(struct disk_stat *last, struct disk_stat *current, unsigned ticks)
{
	int sb = 512;
	int rows = 0;
	for (int i = 0; i < DISK_STAT_NUM_MAX; i++) {
		if (!current[i].rx && !current[i].wx)
			continue;
		fputs(current[i].name, stderr);
		fputs(":", stderr);
		for (int c = strlen(current[i].name); c < 4; c++)
			fputs(" ", stderr);
		int j = 0;
		while (j < DISK_STAT_NUM_MAX && strcmp(last[j].name, current[i].name))
			j++;
		if (j < DISK_STAT_NUM_MAX) {
			fputs(" rx=", stderr);
			aligned_1024((1000 * sb * (current[i].rx - last[j].rx) + ticks / 2) / ticks);
			fputs("b/s wx=", stderr);
			aligned_1024((1000 * sb * (current[i].wx - last[j].wx) + ticks / 2) / ticks);
			fputs("b/s", stderr);
		}
		fputs(" total: rx=", stderr);
		aligned_1024(sb * current[i].rx);
		fputs("b wx=", stderr);
		aligned_1024(sb * current[i].wx);
		fputs("b\n", stderr);
		rows++;
	}
	return rows;
}

int handle_disk_stat(unsigned ticks)
{
	static struct disk_stat last_disk_stat[DISK_STAT_NUM_MAX], current_disk_stat[DISK_STAT_NUM_MAX];
	update_disk_stat(current_disk_stat);
	int rows = show_disk_stat(last_disk_stat, current_disk_stat, ticks);
	copy_disk_stat(last_disk_stat, current_disk_stat);
	return rows;
}

