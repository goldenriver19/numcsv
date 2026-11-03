CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
LDLIBS ?= -lm

numcsv: src/main.c
	$(CC) $(CFLAGS) -o $@ src/main.c $(LDLIBS)

.PHONY: all clean test
all: numcsv

test: numcsv
	sh tests/test_numcsv.sh ./numcsv

clean:
	rm -f numcsv
