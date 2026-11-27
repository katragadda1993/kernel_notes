CC ?= cc
CFLAGS ?= -O2 -g -Wall -Wextra -Werror -std=gnu11

.PHONY: all clean
all: syscall_lab

syscall_lab: syscall_lab.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f syscall_lab
