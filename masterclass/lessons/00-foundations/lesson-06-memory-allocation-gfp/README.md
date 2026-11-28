# Lesson 06 Lab — Kernel Memory Allocation

## Files

- lesson06_alloc.c: allocates a small object, an array, a vmalloc buffer and one typed SLUB-cache object; schedules a workqueue allocation and one timer-context allocation.
- Makefile: external-module Kbuild wrapper.
- memory-observe.sh: read-only snapshot of memory, buddy and slab counters.

## Build

Use the exact configured build tree for the kernel that will load the module:

```sh
make KDIR=/path/to/exact/kernel/build clean all
modinfo ./lesson06_alloc.ko
readelf -h ./lesson06_alloc.ko
```

The module must not be loaded into a different kernel release/configuration merely because it compiled.

## Runtime in a disposable matching VM

```sh
sudo insmod ./lesson06_alloc.ko array_count=32 delay_ms=100
dmesg | tail -30
sudo rmmod lesson06_alloc
dmesg | tail -30
```

Expected log sequence includes init, worker, timer (if allocation succeeds), and exit. The timer callback is a non-sleeping context and uses GFP_ATOMIC; failure is handled. This is a teaching example, not a recommendation to allocate on every real interrupt.

## Observe allocator state

```sh
chmod +x ./memory-observe.sh
./memory-observe.sh
```

The script is read-only. It prints /proc/meminfo, /proc/buddyinfo, /proc/pagetypeinfo, /proc/slabinfo and a sample of /sys/kernel/slab where available.

## Validation performed in the authoring environment

- Built successfully with Kbuild against /usr/src/linux-headers-6.12.107+deb13-amd64.
- modinfo confirmed module name, GPL license, description, parameters and 6.12.107 vermagic.
- readelf -h identified an ELF64 x86-64 relocatable module.
- memory-observe.sh passed shell syntax validation and ran on host kernel 6.18.57.
- The module was **not loaded**: the host kernel is 6.18.57, not the 6.12.107 build target.
- No allocation-injection, memory-pressure, runtime timer/workqueue, Sparse, v7.4-rc4 build, arm64 or hardware result is claimed.
