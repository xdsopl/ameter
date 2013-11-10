#! /usr/bin/python

import time
import datetime
import curses
import signal
import sys
import os

quit=0
def quit_func(signal, frame):
	global quit
	quit=1
signal.signal(signal.SIGINT, quit_func)

stdscr = curses.initscr()
rows, cols = stdscr.getmaxyx()
pad = curses.newpad(rows, cols)
curses.noecho()
pad.clear()
pad.addstr("""
ameter - stat system components in a simple terminal
Written in 2011 by <Ahmet Inan> <xdsopl@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
""")
pad.refresh(0, 0, 0, 0, rows-1, cols-1)

cpu_name = 0
cpu_user = 1
cpu_nice = 2
cpu_sys = 3
cpu_idle = 4
cpu_wait = 5
cpu_irq = 6

disk_load = []
net_load = []
dev_name = 0
stat_rx = 1
stat_wx = 2
stat_tx = 2
load_rx = 1
load_wx = 2
load_tx = 2
total_rx = 3
total_wx = 4
total_tx = 4

mem_total = 0
mem_used = 0
mem_free = 1
mem_buff = 2
mem_cache = 3

swap_total = 0
swap_used = 0
swap_free = 1
swap_in = 2
swap_out = 3

def fu(n):
	for p in '', 'k', 'm', 'g':
		if n >= 10000:
			n /= 1000.
		else:
			break
	return str(int(n + 0.5))+p


def fui(n):
	for p in '', 'ki', 'mi', 'gi':
		if n >= 10240:
			n /= 1024.
		else:
			break
	return str(int(n + 0.5))+p

init=1
while not quit:
	cpu_stat = []
	irq_stat = []
	disk_stat = []
	net_stat = []
	mem_stat = [0]*4
	swap_stat = [0]*4

	for line in open('/proc/stat'):
		if line.startswith('cpu'):
			if line[3].isdigit():
				tmp = line.split()
				cpu_stat += [[tmp[0]] + [int(tmp[1])] + [int(tmp[2])] + [int(tmp[3])] + [int(tmp[4])] + [int(tmp[5])] + [int(tmp[6]) + int(tmp[7])]]
		if line.startswith('intr'):
			irq_cnt = 0
			for tmp in line[5:].split()[1:]:
				irq_cnt += 1
				irq_stat += [int(tmp)]

	for line in open('/proc/diskstats'):
		tmp = line.split()
		if tmp[2].startswith('hd') or tmp[2].startswith('sd') or tmp[2].startswith('sr'):
			if len(tmp[2]) < 4:
				disk_stat += [[tmp[2]] + [int(tmp[5])] + [int(tmp[9])]]
		if tmp[2].startswith('ps3'):
			if len(tmp[2]) < 6:
				disk_stat += [[tmp[2]] + [int(tmp[5])] + [int(tmp[9])]]

	for line in open('/proc/net/dev'):
		if ':' == line[6]:
			tmp = line[7:].split()
			net_stat += [[line[:6].lstrip()] + [int(tmp[0])] + [int(tmp[8])]]

	for line in open('/proc/meminfo'):
		if line.startswith('MemTotal'):
			mem_stat[mem_total] = int(line.split()[1]) * 1024
		if line.startswith('MemFree'):
			mem_stat[mem_free] = int(line.split()[1]) * 1024
		if line.startswith('Buffers'):
			mem_stat[mem_buff] = int(line.split()[1]) * 1024
		if line.startswith('Cached'):
			mem_stat[mem_cache] = int(line.split()[1]) * 1024
		if line.startswith('SwapTotal'):
			swap_stat[swap_total] = int(line.split()[1]) * 1024
		if line.startswith('SwapFree'):
			swap_stat[swap_free] = int(line.split()[1]) * 1024

	for line in open('/proc/vmstat'):
		if line.startswith('pswpin'):
			swap_stat[swap_in] = int(line.split()[1])
		if line.startswith('pswpout'):
			swap_stat[swap_out] = int(line.split()[1])

	cur_time = time.time()

	cpu_load = []
	irq_load = []
	cpu_count = 0
	if not init:
		diff_time = cur_time - last_time
		for cpu in cpu_stat:
			for last in cpu_last:
				if cpu[cpu_name] == last[cpu_name]:
					tmp = [cpu[cpu_name]]
					for stat in cpu_user, cpu_nice, cpu_sys, cpu_idle, cpu_wait, cpu_irq:
						tmp += [int((cpu[stat] - last[stat]) / diff_time + 0.5)]
					cpu_load += [tmp]
					cpu_count += 1

		for n in range(irq_cnt):
			if irq_last[n] < irq_stat[n]:
				irq_load += [[n, int((irq_stat[n] - irq_last[n]) / diff_time + 0.5)]]

		disk_load = []
		for dev in disk_stat:
			if dev[stat_rx] == 0 and dev[stat_wx] == 0:
				continue
			for last in disk_last:
				if dev[dev_name] == last[dev_name]:
					tmp = [dev[dev_name]]
					for stat in stat_rx, stat_wx:
						tmp += [int(512 * (dev[stat] - last[stat]) / diff_time + 0.5)]
					tmp += [int(512 * dev[stat_rx]), int(512 * dev[stat_wx])]
					disk_load += [tmp]

		net_load = []
		for dev in net_stat:
			if dev[stat_rx] == 0 and dev[stat_tx] == 0:
				continue
			for last in net_last:
				if dev[dev_name] == last[dev_name]:
					tmp = [dev[dev_name]]
					for stat in stat_rx, stat_tx:
						tmp += [int((dev[stat] - last[stat]) / diff_time + 0.5)]
					tmp += [int(dev[stat_rx]), int(dev[stat_tx])]
					net_load += [tmp]

		swap_util = [int(swap_stat[swap_total] - swap_stat[swap_free]), swap_stat[swap_free]]
		for stat in swap_in, swap_out:
			swap_util += [int((swap_stat[stat] - swap_last[stat]) / diff_time + 0.5)]


		mem_util = [int(mem_stat[mem_total] - mem_stat[mem_free] - mem_stat[mem_buff] - mem_stat[mem_cache])] + mem_stat[1:]

		pad.clear()
		pad.addstr("date:  ")
		pad.addstr(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		pad.addstr("\n\n")

		columns = int(int(cpu_count + 3) / 4)
		rows = int(40 / columns)
		delta = rows / 100.0;
		counter = 0;
		for cpu in cpu_load:
			user=int(delta * cpu[cpu_user] + 0.5)
			nice=int(delta * cpu[cpu_nice] + 0.5)
			if nice + user > rows:
				nice -= nice + user - rows
			sys=int(delta * cpu[cpu_sys] + 0.5)
			if sys + nice + user > rows:
				sys -= sys + nice + user - rows
			wait=int(delta * cpu[cpu_wait] + 0.5)
			if wait + sys + nice + user > rows:
				wait -= wait + sys + nice + user - rows
			irq=int(delta * cpu[cpu_irq] + 0.5)
			if irq + wait + sys + nice + user > rows:
				irq -= irq + wait + sys + nice + user - rows
			idle=rows-user-nice-sys-wait-irq
			pad.addstr(cpu[cpu_name]+":"+" "*int(6-len(cpu[cpu_name]))+"["+"u"*int(user)+"n"*int(nice)+"s"*int(sys)+"w"*int(wait)+"q"*int(irq)+" "*int(idle)+"]")
			counter += 1
			if counter % columns == 0:
				pad.addstr("\n")
			else:
				pad.addstr(" ")
		if counter % columns != 0:
			pad.addstr("\n")

		pad.addstr("\n")

		pad.addstr("irq/s:")
		for irq in irq_load:
			pad.addstr(" q"+str(irq[0])+"="+fu(irq[1]))
		pad.addstr("\n")

		pad.addstr("\n")

		used=int(25.0 * mem_util[mem_used] / mem_stat[mem_total] + 0.5)
		buff=int(25.0 * mem_util[mem_buff] / mem_stat[mem_total] + 0.5)
		if buff + used > 25:
			buff -= buff + used - 25
		cache=int(25.0 * mem_util[mem_cache] / mem_stat[mem_total] + 0.5)
		if cache + buff + used > 25:
			cache -= cache + buff + used - 25
		free=25-used-buff-cache
		pad.addstr("mem:   ["+"u"*int(used)+"b"*int(buff)+"c"*int(cache)+" "*int(free)+"]")
		used=fui(mem_util[mem_used])
		buff=fui(mem_util[mem_buff])
		cache=fui(mem_util[mem_cache])
		free=fui(mem_util[mem_free])
		pad.addstr(" u="+used+"b b="+buff+"b c="+cache+"b f="+free+"b\n")
		if swap_stat[swap_total] > 0:
			used=int(25.0 * swap_util[swap_used] / swap_stat[swap_total])
			free=25-used
			pad.addstr("swap:  ["+"u"*used+" "*free+"]")
			used=fui(swap_util[swap_used])
			free=fui(swap_util[swap_free])
			input=fu(swap_util[swap_in])
			output=fu(swap_util[swap_out])
			pad.addstr(" u="+used+"b f="+free+"b i="+input+"p/s o="+output+"p/s\n")

		pad.addstr("\n")

		for dev in net_load:
			pad.addstr(dev[dev_name]+":"+" "*(6-len(dev[dev_name])))
			pad.addstr("rx="+fui(dev[load_rx]).rjust(7)+"b/s tx="+fui(dev[load_tx]).rjust(7)+"b/s ")
			pad.addstr("total: rx="+fui(dev[total_rx])+"b tx="+fui(dev[total_tx])+"b\n")

		pad.addstr("\n")

		for dev in disk_load:
			pad.addstr(dev[dev_name]+":"+" "*(6-len(dev[dev_name])))
			pad.addstr("rx="+fui(dev[load_rx]).rjust(7)+"b/s wx="+fui(dev[load_wx]).rjust(7)+"b/s ")
			pad.addstr("total: rx="+fui(dev[total_rx])+"b wx="+fui(dev[total_wx])+"b\n")

		rows, cols = stdscr.getmaxyx()
		pad.refresh(0, 0, 0, 0, rows-1, cols-1)

	last_time = cur_time
	cpu_last = cpu_stat
	irq_last = irq_stat
	disk_last = disk_stat
	net_last = net_stat
	swap_last = swap_stat

	init = 0
	time.sleep(2)

curses.echo()
curses.endwin()

