CFLAGS = -std=c99 -O2 -W -Wall -Wextra -D_GNU_SOURCE=1
LDFLAGS = -lrt

ameter: ameter.c

test: ameter
	./ameter

clean:
	rm -f ameter

