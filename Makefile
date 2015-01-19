CFLAGS = -std=c99 -O2 -W -Wall -Wextra -D_GNU_SOURCE=1
LDLIBS = -lrt -lcurses

ameter: ameter.o cpu_stat.o net_stat.o disk_stat.o mem_info.o utils.o

test: ameter
	./ameter

clean:
	rm -f ameter *.o

