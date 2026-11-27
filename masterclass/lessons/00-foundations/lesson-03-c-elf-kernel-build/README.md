# Lesson 03 Lab — C layout, ELF inspection and Kbuild

## Requirements

- GCC or Clang
- GNU Make
- binutils (readelf, objdump, nm)
- For the module exercise: a configured kernel build tree matching the intended target kernel

## Lab A: userspace C and ELF

~~~bash
make clean all
./elf_layout_O0
./elf_layout_O2
readelf -h -S -s -r elf_layout_O0.o
objdump -dr elf_layout_O0.o
nm -S elf_layout_O0.o
~~~

The sample output on the validated x86-64 ABI is:

~~~text
pointer_size=8
dma_desc_size=16 addr_off=0 length_off=8 control_off=12
packed_header_size=5 value_off=1
desc_addr=0x12345000 length=4096 control=0x00000405
wire_tag=0xa5 wire_value=0x11223344
marker=ELF_SECTION_MARKER
~~~

Expected inspection points:
- elf_layout_O0.o is ELF64, little-endian, x86-64, type REL.
- .rodata.lesson contains the custom marker.
- .rela.text contains relocations for external printf calls and references to constants.
- nm shows lesson_marker, make_control and main.
- The -O0 and -O2 binaries print the same logical output, but their generated machine code may differ.

## Lab B: external kernel module

~~~bash
KDIR=/path/to/configured/kernel/build
make -C "$KDIR" M="$PWD/kbuild" modules
file kbuild/lesson03_hello.ko
readelf -h kbuild/lesson03_hello.ko
modinfo kbuild/lesson03_hello.ko
~~~

The module must be built against the exact target kernel build tree. Do not load an untrusted module or load a module into a kernel whose version/configuration does not match the build.

## Validation record for this repository lesson

Validated in an x86-64 container on 2026-11-28:
- Host kernel: 6.18.57 x86_64
- GCC: 14.2.0
- GNU Make: 4.4.1
- GNU binutils: 2.44
- Clang: 17.0.0
- Userspace lab: compiled at -O0 and -O2; both executables ran and produced the expected output.
- ELF inspection: readelf, objdump and nm showed the expected ELF64 relocatable object, custom section, symbols and relocation entries.
- External module: compiled successfully against /usr/src/linux-headers-6.12.107+deb13-amd64; modinfo reported vermagic 6.12.107+deb13-amd64.
- The .ko was not loaded because the running host kernel (6.18.57) did not match the module build headers (6.12.107). This is intentional and correct.
- Sparse was not installed, so Sparse annotation checking was not run.
- No full v7.4-rc4 kernel build, arm64 cross-build or QEMU boot was performed.

## Clean up

~~~bash
make clean
make -C "$KDIR" M="$PWD/kbuild" clean
~~~
