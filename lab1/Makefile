CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS :=

.PHONY: all clean

all: mycat mygrep

mycat: mycat.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

mygrep: mygrep.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f mycat mygrep