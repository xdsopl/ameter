/*
ameter - stat system components in a simple terminal
Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

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

void readable_1024(unsigned long long value)
{
	char *prefix[] = { "", "ki", "mi", "gi", "ti", "pi" };
	unsigned i = 0;
	while (i < sizeof(prefix) / sizeof(*prefix)) {
		if (value < 10240)
			break;
		value /= 1024;
		i++;
	}
	fprintf(stderr, "%llu%s", value, prefix[i]);
}

void aligned_1024(unsigned long long value)
{
	char *prefix[] = { "", "ki", "mi", "gi", "ti", "pi" };
	unsigned i = 0;
	while (i < sizeof(prefix) / sizeof(*prefix)) {
		if (value < 10240)
			break;
		value /= 1024;
		i++;
	}
	if (!i)
		fputs("  ", stderr);
	fprintf(stderr, "%5llu%s", value, prefix[i]);
}

#define CPU_NUM_MAX (512)
#define CPU_ONLINE (1 << 0)
#define CPU_USER (0)
#define CPU_NICE (1)
#define CPU_SYSTEM (2)
#define CPU_IOWAIT (3)
#define CPU_IRQ (4)
#define CPU_IDLE (5)
#define CPU_STAT_MAX (6)

struct cpu_stat {
	unsigned long long stats[CPU_STAT_MAX], flags;
};

void parse_cpu_stat(struct cpu_stat *cpus, char *str)
{
	struct cpu_stat cpu;
	int num;
	unsigned long long softirq, steal, guest, guest_nice;
	int ret = sscanf(str, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
		&num,
		&cpu.stats[CPU_USER],
		&cpu.stats[CPU_NICE],
		&cpu.stats[CPU_SYSTEM],
		&cpu.stats[CPU_IDLE],
		&cpu.stats[CPU_IOWAIT],
		&cpu.stats[CPU_IRQ],
		&softirq,
		&steal,
		&guest,
		&guest_nice);

	cpu.stats[CPU_IRQ] += softirq;
	cpu.stats[CPU_SYSTEM] += steal;
	cpu.stats[CPU_USER] += guest;
	cpu.stats[CPU_NICE] += guest_nice;

	cpu.flags |= CPU_ONLINE;

	if (ret == 11 && 0 <= num && num < CPU_NUM_MAX)
		cpus[num] = cpu;
}

void update_cpu_stat(struct cpu_stat *cpus)
{
	memset(cpus, 0, sizeof(struct cpu_stat) * CPU_NUM_MAX);
	FILE *proc_stat = fopen("/proc/stat", "r");
	char str[4096];
	while (fgets(str, sizeof(str), proc_stat)) {
		if (!strncmp(str, "cpu", 3) && str[3] != ' ')
			parse_cpu_stat(cpus, str);
	}
	fclose(proc_stat);
}

int show_cpu_stat(struct cpu_stat *last, struct cpu_stat *current, int term_width, int compact)
{
	(void)compact;
	int online = 0;
	for (int i = 0; i < CPU_NUM_MAX; i++)
		online += (current[i].flags & CPU_ONLINE) ? 1 : 0;
	int columns = online <= 16 ? online <= 8 ? online <= 4 ? 1 : 2 : 4 : 16;
	int matrix_view = online > 16;
	int width = term_width / columns;
	int rows = (online + columns - 1) / columns;
	if (matrix_view) {
		width--;
	} else {
		width -= strlen("cpuN: [] ") + (online < 10 ? 0 : 1);
	}
	int cur_col = 0;
	int init = 1;
	for (int i = 0; i < CPU_NUM_MAX; i++) {
		if (!(last[i].flags & current[i].flags & CPU_ONLINE))
			continue;
		init = 0;
		int diff[CPU_STAT_MAX];
		for (int s = 0; s < CPU_STAT_MAX; s++)
			diff[s] = current[i].stats[s] - last[i].stats[s];
		int sum = 0;
		for (int s = 0; s < CPU_STAT_MAX; s++)
			sum += diff[s];
		for (int s = 0; s < CPU_STAT_MAX; s++)
			diff[s] = (width * diff[s] + sum / 2) / sum;
		sum = 0;
		for (int s = 0; s < CPU_STAT_MAX; s++)
			sum += diff[s];
		diff[CPU_IDLE] += width - sum;
		char type[CPU_STAT_MAX] = { 'u', 'n', 's', 'w', 'q', ' ' };
		if (!matrix_view)
			fprintf(stderr, "cpu%d:%s[", i, i < 10 && online >= 10 ? "  " : " ");
		for (int s = 0; s < CPU_STAT_MAX; s++)
			for (int c = 0; c < diff[s]; c++)
				fputc(type[s], stderr);
		if (matrix_view) {
			fputc('|', stderr);
			if (++cur_col >= columns) {
				fputc('\n', stderr);
				cur_col = 0;
			}
		} else {
			fputc(']', stderr);
			if (++cur_col < columns) {
				fputc(' ', stderr);
			} else {
				fputc('\n', stderr);
				cur_col = 0;
			}
		}
	}
	if (cur_col)
		fputc('\n', stderr);
	if (init)
		fprintf(stderr, "initializing %d cpus ([u]ser, [n]ice, [s]ystem, io[w]ait, ir[q]) ..\n", online);
	return rows;
}

void copy_cpu_stat(struct cpu_stat *dst, struct cpu_stat *src)
{
	memcpy(dst, src, sizeof(struct cpu_stat) * CPU_NUM_MAX);
}

int handle_cpu_stat(int term_width, int compact)
{
	static struct cpu_stat last_cpu_stat[CPU_NUM_MAX], current_cpu_stat[CPU_NUM_MAX];
	update_cpu_stat(current_cpu_stat);
	int rows = show_cpu_stat(last_cpu_stat, current_cpu_stat, term_width, compact);
	copy_cpu_stat(last_cpu_stat, current_cpu_stat);
	return rows;
}

struct mem_info {
	unsigned long total, free, buffers, cached, swap_total, swap_free;
};

void update_mem_info(struct mem_info *mem)
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

int show_mem_info(struct mem_info *mem, int term_width)
{
	unsigned long used = mem->total - mem->free - mem->buffers - mem->cached;
	int width = term_width - 5 * 10 - 7;
	int used_width = (width * used + mem->total / 2) / mem->total;
	int buffers_width = (width * mem->buffers + mem->total / 2) / mem->total;
	int cached_width = (width * mem->cached + mem->total / 2) / mem->total;
	int free_width = width - used_width - buffers_width - cached_width;
	fprintf(stderr, "mem: [");
	for (int i = 0; i < used_width; i++)
		fputc('u', stderr);
	for (int i = 0; i < buffers_width; i++)
		fputc('b', stderr);
	for (int i = 0; i < cached_width; i++)
		fputc('c', stderr);
	for (int i = 0; i < free_width; i++)
		fputc(' ', stderr);
	fprintf(stderr, "] t=");
	readable_1024(1024 * mem->total);
	fprintf(stderr, "b u=");
	readable_1024(1024 * used);
	fprintf(stderr, "b b=");
	readable_1024(1024 * mem->buffers);
	fprintf(stderr, "b c=");
	readable_1024(1024 * mem->cached);
	fprintf(stderr, "b f=");
	readable_1024(1024 * mem->free);
	fprintf(stderr, "b\n");
	return 1;
}

int handle_mem_info(int term_width)
{
	struct mem_info mem;
	update_mem_info(&mem);
	return show_mem_info(&mem, term_width);
}

#define NET_DEV_NUM_MAX (8)

struct net_dev {
	char name[8];
	unsigned long long rx, tx;
};

void parse_net_dev(struct net_dev *dev, char *str)
{
	int i;
	for (i = 0; str[i] == ' ' && i < 6; i++);
	memcpy(dev->name, str + i, 6 - i);
	dev->name[6 - i] = 0;
	str += 7;
	unsigned long long tmp[7];
	sscanf(str, "%llu %llu %llu %llu %llu %llu %llu %llu %llu",
		&dev->rx, &tmp[0], &tmp[1], &tmp[2], &tmp[3],
		&tmp[4], &tmp[5], &tmp[6], &dev->tx);
}

void update_net_dev(struct net_dev *devs)
{
	memset(devs, 0, sizeof(struct net_dev) * NET_DEV_NUM_MAX);
	FILE *proc_net_dev = fopen("/proc/net/dev", "r");
	char str[4096];
	int i = 0;
	while (i < NET_DEV_NUM_MAX && fgets(str, sizeof(str), proc_net_dev)) {
		if (strlen(str) > 6 && str[6] == ':')
			parse_net_dev(devs + i++, str);
	}
	fclose(proc_net_dev);
}

void copy_net_dev(struct net_dev *dst, struct net_dev *src)
{
	memcpy(dst, src, sizeof(struct net_dev) * NET_DEV_NUM_MAX);
}

int show_net_dev(struct net_dev *last, struct net_dev *current, unsigned ticks)
{
	int rows = 0;
	for (int i = 0; i < NET_DEV_NUM_MAX; i++) {
		if (!current[i].rx && !current[i].tx)
			continue;
		fputs(current[i].name, stderr);
		fputs(":", stderr);
		for (int c = strlen(current[i].name); c < 4; c++)
			fputs(" ", stderr);
		int j = 0;
		while (j < NET_DEV_NUM_MAX && strcmp(last[j].name, current[i].name))
			j++;
		if (j < NET_DEV_NUM_MAX) {
			fputs(" rx=", stderr);
			aligned_1024((1000 * (current[i].rx - last[j].rx) + ticks / 2) / ticks);
			fputs("b/s tx=", stderr);
			aligned_1024((1000 * (current[i].tx - last[j].tx) + ticks / 2) / ticks);
			fputs("b/s", stderr);
		}
		fputs(" total: rx=", stderr);
		readable_1024(current[i].rx);
		fputs("b tx=", stderr);
		readable_1024(current[i].tx);
		fputs("b\n", stderr);
		rows++;
	}
	return rows;
}

int handle_net_dev(unsigned ticks)
{
	static struct net_dev last_net_dev[NET_DEV_NUM_MAX], current_net_dev[NET_DEV_NUM_MAX];
	update_net_dev(current_net_dev);
	int rows = show_net_dev(last_net_dev, current_net_dev, ticks);
	copy_net_dev(last_net_dev, current_net_dev);
	return rows;
}

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
		readable_1024(sb * current[i].rx);
		fputs("b wx=", stderr);
		readable_1024(sb * current[i].wx);
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

int main()
{
	int compact = 0;
	while (1) {
		fprintf(stderr, "\E[H\E[2J");
		int term_width = 80;
		int term_height = 24;
		int rows = 1;
		fputs(string_time("%F %T\n"), stderr);
		if (!compact)
			fputc('\n', stderr);
		rows += handle_cpu_stat(term_width, compact);
		if (!compact)
			fputc('\n', stderr);
		rows += handle_mem_info(term_width);
		if (!compact)
			fputc('\n', stderr);
		unsigned ticks = get_ticks();
		rows += handle_net_dev(ticks);
		if (!compact)
			fputc('\n', stderr);
		rows += handle_disk_stat(ticks);
		if (rows > term_height)
			compact = 1;
		sleep(3);
	}
	return 0;
}

