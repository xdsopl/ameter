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

#define NET_DEV_NUM_MAX (8)

struct net_stat {
	char name[8];
	unsigned long long rx, tx;
};

static int parse_net_stat(struct net_stat *dev, char *str)
{
	int i;
	for (i = 0; str[i] == ' ' && i < 6; i++);
	if (i > 4)
		return 0;
	memcpy(dev->name, str + i, 6 - i);
	dev->name[6 - i] = 0;
	str += 7;
	unsigned long long tmp[7];
	int ret = sscanf(str, "%llu %llu %llu %llu %llu %llu %llu %llu %llu",
		&dev->rx, &tmp[0], &tmp[1], &tmp[2], &tmp[3],
		&tmp[4], &tmp[5], &tmp[6], &dev->tx);
	if (ret != 9) {
		memset(dev, 0, sizeof(struct net_stat));
		return 0;
	}
	return 1;
}

static void update_net_stat(struct net_stat *devs)
{
	memset(devs, 0, sizeof(struct net_stat) * NET_DEV_NUM_MAX);
	FILE *proc_net_dev = fopen("/proc/net/dev", "r");
	char str[4096];
	int i = 0;
	while (i < NET_DEV_NUM_MAX && fgets(str, sizeof(str), proc_net_dev)) {
		if (strlen(str) > 6 && str[6] == ':')
			i += parse_net_stat(devs + i, str);
	}
	fclose(proc_net_dev);
}

static void copy_net_stat(struct net_stat *dst, struct net_stat *src)
{
	memcpy(dst, src, sizeof(struct net_stat) * NET_DEV_NUM_MAX);
}

static int show_net_stat(struct net_stat *last, struct net_stat *current, unsigned ticks)
{
	int rows = 0;
	for (int i = 0; i < NET_DEV_NUM_MAX; i++) {
		if (!current[i].rx && !current[i].tx)
			continue;
		addstr(current[i].name);
		addstr(":");
		for (int c = strlen(current[i].name); c < 4; c++)
			addstr(" ");
		int j = 0;
		while (j < NET_DEV_NUM_MAX && strcmp(last[j].name, current[i].name))
			j++;
		if (j < NET_DEV_NUM_MAX) {
			addstr(" rx=");
			aligned_1024((1000 * (current[i].rx - last[j].rx) + ticks / 2) / ticks);
			addstr("b/s tx=");
			aligned_1024((1000 * (current[i].tx - last[j].tx) + ticks / 2) / ticks);
			addstr("b/s");
		}
		addstr(" total: rx=");
		aligned_1024(current[i].rx);
		addstr("b tx=");
		aligned_1024(current[i].tx);
		addstr("b\n");
		rows++;
	}
	return rows;
}

int handle_net_stat(unsigned ticks)
{
	static struct net_stat last_net_stat[NET_DEV_NUM_MAX], current_net_stat[NET_DEV_NUM_MAX];
	update_net_stat(current_net_stat);
	int rows = show_net_stat(last_net_stat, current_net_stat, ticks);
	copy_net_stat(last_net_stat, current_net_stat);
	return rows;
}

