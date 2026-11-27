CC ?= gcc
CFLAGS_COMMON := -std=gnu11 -Wall -Wextra -Werror -Wformat=2

.PHONY: all run inspect clean
all: elf_layout_O0 elf_layout_O2

elf_layout_O0.o: elf_layout.c
	$(CC) $(CFLAGS_COMMON) -O0 -g -fno-pie -c $< -o $@

elf_layout_O2.o: elf_layout.c
	$(CC) $(CFLAGS_COMMON) -O2 -g -fno-pie -c $< -o $@

elf_layout_O0: elf_layout_O0.o
	$(CC) -no-pie $< -o $@

elf_layout_O2: elf_layout_O2.o
	$(CC) -no-pie $< -o $@

run: all
	./elf_layout_O0

inspect: all
	readelf -h -S -s -r elf_layout_O0.o
	objdump -dr elf_layout_O0.o
	nm -S elf_layout_O0.o

clean:
	rm -f elf_layout_O0.o elf_layout_O2.o elf_layout_O0 elf_layout_O2
