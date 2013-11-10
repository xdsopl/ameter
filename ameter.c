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

void print_copyright()
{
	fputs(	"ameter - stat system components in a simple terminal\n"
		"Written in 2013 by <Ahmet Inan> <xdsopl@googlemail.com>\n"
		"To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.\n"
		"You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>."
	, stderr);
}

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

void show_cpu_stat(struct cpu_stat *last, struct cpu_stat *current)
{
	int online = 0;
	for (int i = 0; i < CPU_NUM_MAX; i++)
		online += (last[i].flags & current[i].flags & CPU_ONLINE) ? 1 : 0;
	if (!online)
		return;
	int term_width = 80;
	int columns = online <= 16 ? online <= 8 ? online <= 4 ? 1 : 2 : 4 : 16;
	int matrix_view = online > 16;
	int width = term_width / columns;
	if (matrix_view) {
		width--;
		fprintf(stderr, "showing %d cpus ([u]ser, [n]ice, [s]ystem, io[w]ait, ir[q]):\n", online);
	} else {
		width -= strlen("cpuN: [] ") + (online < 10 ? 0 : 1);
	}
	int cur_col = 0;
	for (int i = 0; i < CPU_NUM_MAX; i++) {
		if (!(last[i].flags & current[i].flags & CPU_ONLINE))
			continue;
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
}

void copy_cpu_stat(struct cpu_stat *dst, struct cpu_stat *src)
{
	memcpy(dst, src, sizeof(struct cpu_stat) * CPU_NUM_MAX);
}

void handle_cpu_stat()
{
	static struct cpu_stat last_cpu_stat[CPU_NUM_MAX], current_cpu_stat[CPU_NUM_MAX];
	update_cpu_stat(current_cpu_stat);
	show_cpu_stat(last_cpu_stat, current_cpu_stat);
	copy_cpu_stat(last_cpu_stat, current_cpu_stat);
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

void show_mem_info(struct mem_info *mem)
{
	unsigned long used = mem->total - mem->free - mem->buffers - mem->cached;
	fprintf(stderr, "mem: u = %lukib b = %lukib c = %lukib f = %lukib\n",
			used, mem->buffers, mem->cached, mem->free);
}

void handle_mem_info()
{
	struct mem_info mem;
	update_mem_info(&mem);
	show_mem_info(&mem);
}

int main()
{
	fprintf(stderr, "\E[H\E[2J");
	print_copyright();
	while (1) {
		unsigned ticks = get_ticks();
		(void)ticks;
		handle_cpu_stat();
		handle_mem_info();
		sleep(3);
		fprintf(stderr, "\E[H\E[2J");
	}
	return 0;
}

