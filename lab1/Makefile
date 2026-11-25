CC = cc
CPPFLAGS = -D_GNU_SOURCE
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

all: parent child

parent: parent.c io.c io.h
	$(CC) $(CPPFLAGS) $(CFLAGS) parent.c io.c -o $@

child: child.c io.c io.h
	$(CC) $(CPPFLAGS) $(CFLAGS) child.c io.c -o $@

clean:
	$(RM) parent child

.PHONY: all clean
