# Lesson 02 — Linux Boot: From Firmware Handoff to PID 1

**Stage:** 0 — Systems foundations; bridge into Stage 1 — Kernel architecture and boot  
**Duration:** 75–90 minutes  
**Level:** Beginner → advanced source reading  
**Source baseline:** Linux mainline tag [v7.4-rc4](https://github.com/torvalds/linux/tree/v7.4-rc4)  
**Lab:** [build-initramfs.sh](lesson-02-linux-boot-and-early-init/build-initramfs.sh) · [init](lesson-02-linux-boot-and-early-init/init)

## 1. Learning objectives

After this lesson, you should be able to:

1. Explain the contract between firmware, bootloader and the Linux kernel.
2. Trace the x86-64 compressed-kernel path into the architecture-independent C entry point.
3. Trace the arm64 Image entry, FDT handoff, initial page tables and MMU enable sequence.
4. Explain what happens inside start_kernel(), rest_init(), kernel_init() and kernel_init_freeable().
5. Distinguish initrd, initramfs, rootfs, rdinit=, init= and root=.
6. Explain initcall ordering, linker sections and why __init memory can be reclaimed.
7. Debug failures before the first userspace process, including missing console, missing root device, missing /init and early driver dependencies.
8. Explain why a bootloader must quiesce DMA-capable devices before handing memory ownership to the kernel.

## 2. Why boot is a protocol, not a single function

The kernel cannot assume that firmware has left the CPU, memory controller, interrupt controller, caches or devices in the exact state Linux wants. A bootloader must establish a documented handoff state and pass machine information. The kernel then performs architecture-specific setup before entering common C code.

A useful high-level model:

~~~text
Power-on / reset
      |
      v
Boot ROM / platform firmware
      |
      v
UEFI / BIOS / TF-A / platform boot firmware
      |
      v
Bootloader (GRUB, U-Boot, EFI stub, vendor loader)
      |
      +---------------------------+
      |                           |
      v                           v
x86-64                         arm64
bzImage setup                 Image header + DTB/ACPI
compressed entry              primary_entry
decompress / relocate         CPU + page-table setup
x86_64_start_kernel           MMU enable / virtual mapping
      |                           |
      +-------------+-------------+
                    |
                    v
              start_kernel()
                    |
          architecture + MM + IRQ
          scheduler + time + console
                    |
                rest_init()
                    |
          +---------+----------+
          |                    |
      kernel_init            kthreadd
          |
     initcalls / drivers
          |
    wait for initramfs
          |
  rdinit or mount real root
          |
        execve()
          |
       PID 1 userspace
~~~ 

The exact firmware chain is platform-dependent. For example, an arm64 server may pass through secure firmware and UEFI; an embedded SoC may use Boot ROM, TF-A and U-Boot. Do not assume every platform has the same stages.

## 3. The bootloader-to-kernel contract

### 3.1 x86-64: boot protocol and bzImage

The x86 boot protocol is a compatibility interface between bootloaders and the kernel image. It carries information such as boot parameters, memory map, command line, initrd location and loader capabilities.

A modern x86 bzImage contains setup code plus a compressed kernel payload and decompressor. It is not simply a gzip file with a kernel inside.

Pinned source walkthrough:

- [x86 boot protocol documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/x86/boot.rst)
- [x86 compressed 64-bit entry: startup_64()](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/boot/compressed/head_64.S#L282-L480)
- [decompressor: extract_kernel()](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/boot/compressed/misc.c#L407-L500)
- [decompressed kernel C entry](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/kernel/head64.c#L222-L310)
- [x86-64 kernel head assembly](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/kernel/head_64.S)

Conceptual sequence:

1. Firmware or an EFI stub/bootloader loads the kernel setup image and supplies boot parameters.
2. The x86 setup code validates the boot protocol data and prepares the protected-mode/long-mode transition.
3. The compressed image enters startup_64() (or reaches it through the 32-bit startup path).
4. The decompressor establishes the mappings and temporary environment it needs.
5. extract_kernel() parses/sanitizes boot parameters, handles memory placement and decompresses the kernel image.
6. Control jumps to the decompressed kernel entry.
7. x86_64_start_kernel() performs early x86-specific setup and hands off through x86_64_start_reservations().
8. x86_64_start_reservations() reaches start_kernel().

Important distinction: the decompressor and the real kernel are different execution phases. A failure in the compressed image can occur before the normal kernel console or most kernel tracing facilities exist.

### 3.2 arm64: Image entry and the FDT handoff

The arm64 boot protocol is intentionally explicit about the CPU and memory state at entry.

For the v7.4-rc4 source and its boot documentation:

- [arm64 boot protocol](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/arm64/booting.rst)
- [primary_entry() and __primary_switched()](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/head.S#L57-L247)
- [__primary_switch()](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/head.S#L507-L522)
- [arm64 setup_arch()](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/setup.c)

Key handoff points:

- x0 contains the physical address of the device-tree blob (DTB) when booting through the documented FDT path.
- x1–x3 are reserved and must be zero.
- The MMU must be off at the documented kernel entry.
- Interrupts must be masked in PSTATE.DAIF.
- The CPU enters in non-secure EL2 (recommended) or EL1, subject to the boot protocol.
- The bootloader must initialize usable RAM and place the DTB in a valid location.
- DMA-capable devices must be quiesced before the kernel is entered, so stale device transactions cannot corrupt memory that Linux is about to use.

The last requirement is especially important for SoC engineers. If a DSP, PCIe endpoint, storage controller or DMA engine continues writing to a buffer after the bootloader hands control to Linux, the kernel may later allocate that physical memory for unrelated objects. The resulting symptom can look like random memory corruption rather than a bootloader bug.

The arm64 assembly path:

1. primary_entry() preserves the boot arguments and records the FDT pointer.
2. It establishes an early stack and creates the initial identity map.
3. It detects/configures the CPU execution level and performs CPU setup.
4. __primary_switch() enables the MMU and maps/relocates the kernel.
5. __primary_switched() installs the exception vector base, records the FDT pointer and reaches start_kernel().

Unlike x86 bzImage, arm64 does not provide the same in-kernel decompressor path for Image.gz. The bootloader must decompress a compressed arm64 image before entering it; the uncompressed Image is also available.

### 3.3 Device Tree versus ACPI

The boot handoff may describe hardware using Device Tree or ACPI, depending on platform and firmware design. The kernel architecture setup code discovers memory, reserved regions, CPUs, interrupt controllers and platform resources from the supplied firmware description.

For an audio/SoC platform, this can determine whether the kernel can discover:

- the audio controller and its MMIO range;
- interrupt lines and interrupt-controller topology;
- clocks, resets and regulators;
- IOMMU and DMA-coherency properties;
- reserved memory for DSP firmware or shared buffers;
- mailbox and remote-processor relationships.

The DTB is not merely a list of devices. It is part of the boot-time hardware contract.

## 4. The common C initialization path

The architecture-specific code eventually calls start_kernel() in [init/main.c](https://github.com/torvalds/linux/blob/v7.4-rc4/init/main.c#L1043-L1248).

Do not memorize this as one immutable list. The exact order changes as dependencies and subsystems evolve. For v7.4-rc4, important landmarks include:

| Phase | Examples | Why ordering matters |
|---|---|---|
| Establish boot CPU state | set_task_stack_end_magic(), boot_cpu_init() | The initial task and CPU identity must be valid |
| Architecture setup | setup_arch() | Memory map, firmware tables, command line and architecture resources |
| Early memory and traps | mm_core_init_early(), trap_init(), mm_core_init() | Later code relies on memory and exception handling |
| Scheduler and workqueues | sched_init(), workqueue_init_early() | Scheduling and deferred execution must be initialized in stages |
| Interrupt and time setup | early_irq_init(), init_IRQ(), timers_init(), timekeeping_init() | IRQ and timer sources must be ready before normal use |
| Console and diagnostics | console_init(), lockdep_init() | Useful boot diagnostics and lock checking |
| Core subsystems | VFS, credentials, security, namespaces, networking | Prepare the kernel to create and manage tasks and devices |
| Transition | rest_init() | Start the init and kthreadd kernel threads |

A few important observations from the source:

- Local interrupts are disabled early. The kernel performs required setup before enabling them.
- setup_arch() runs before the generic memory-management and subsystem initialization is complete.
- The scheduler is initialized before normal timer interrupt processing begins.
- The console is initialized early enough to capture failures, but the source itself calls out that PCI setup is not yet complete. An early console must not assume every device is ready.
- start_kernel() eventually calls rest_init(); it does not directly execute /sbin/init.

## 5. rest_init(), PID 1 and kthreadd

See [rest_init() in init/main.c](https://github.com/torvalds/linux/blob/v7.4-rc4/init/main.c#L738-L790).

rest_init() creates the kernel_init task first so it obtains PID 1, then creates kthreadd. It releases the init task after kthreadd is ready and the boot CPU enters the idle loop.

Why create kthreadd before allowing init to proceed? The source comment explains that init will need to create kernel threads; scheduling it before kthreadd is ready would be unsafe.

Important identities:

- PID 0 is the boot/idle task.
- PID 1 begins as the kernel_init task and later execs the selected userspace init program.
- kthreadd is the kernel thread manager, normally PID 2.

The PID 1 task is therefore not born as /sbin/init. It begins in kernel code, completes remaining initialization and then replaces its execution image with a userspace program using kernel_execve().

## 6. Initcalls: how built-in subsystems initialize

Built-in code can register initialization functions with macros such as:

- early_initcall()
- core_initcall()
- postcore_initcall()
- arch_initcall()
- subsys_initcall()
- fs_initcall()
- rootfs_initcall()
- device_initcall()
- late_initcall()

The definitions are in [include/linux/init.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/init.h#L185-L316). The linker collects function pointers into ordered sections using [include/asm-generic/vmlinux.lds.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/asm-generic/vmlinux.lds.h#L977-L994). The runtime iteration is in [do_initcalls() and do_initcall_level()](https://github.com/torvalds/linux/blob/v7.4-rc4/init/main.c#L1439-L1518).

A useful simplified order is:

~~~text
early
pure
core
postcore
arch
subsys
fs
rootfs
device
late
~~~

In this source revision, rootfs initcalls occupy a dedicated linker subsection between the fs and device ranges. The runtime loop iterates the range from initcall level 5 to level 6, so rootfs entries are included in that interval; do not assume the printed level name is a separate rootfs phase.

Why levels exist: subsystems have dependencies. A bus, framework or provider often needs to register before a consumer driver can use it. Initcall levels express broad ordering, while the device model and deferred-probe mechanisms handle many runtime dependencies.

Built-in driver initcalls are not the same as module loading. A module's initialization function runs when the module is loaded, which may happen much later from early userspace or the normal root filesystem.

### 6.1 __init memory and lifetime

The __init, __initdata and __initconst annotations place code/data into initialization-only sections. The linker and kernel boot code arrange for those sections to be reclaimed after initialization.

In v7.4-rc4, kernel_init() calls async_synchronize_full() before free_initmem(). This is important: asynchronous initialization work must finish before memory containing init-only code is released.

Driver rule:

- Never store a pointer to an __init function for a callback that may run after boot.
- Never retain an __initdata object for normal runtime use.
- Do not queue asynchronous work that can outlive the init section containing its callback.
- Treat modpost section-mismatch warnings as possible lifetime bugs, not cosmetic noise.

## 7. initrd, initramfs, rootfs and the init program

These names are related but not interchangeable.

| Term | Meaning |
|---|---|
| initrd | Historical initial ramdisk image, presented as a block-device-style filesystem image |
| initramfs | A cpio archive unpacked into the initial rootfs |
| rootfs | The initial root filesystem instance used during boot; it can be backed by ramfs or tmpfs depending on configuration |
| /init | Early userspace program inside initramfs |
| rdinit= | Selects the early userspace init program in the initramfs |
| init= | Selects the final init program after the normal root filesystem path is prepared |
| root= | Identifies the real root device or root source for the kernel's root-mount path |

### 7.1 initramfs path

The kernel's initramfs implementation includes [populate_rootfs()](https://github.com/torvalds/linux/blob/v7.4-rc4/init/initramfs.c#L790-L800). It schedules archive population, potentially asynchronously. kernel_init_freeable() calls wait_for_initramfs() before it tries to execute early userspace.

The path is:

1. Bootloader passes an external archive, or the build embeds one through CONFIG_INITRAMFS_SOURCE.
2. The kernel unpacks the cpio archive into rootfs.
3. kernel_init_freeable() waits for unpacking to finish.
4. If rdinit points to an executable file, the kernel executes it as PID 1.
5. That early userspace program can load modules, discover storage, configure devices and mount the real root filesystem.
6. It then normally switches to the real root and execs the system's final init.

### 7.2 Legacy initrd / kernel root-mount path

If no usable early userspace init is available, the kernel takes the normal root-mount path. In v7.4-rc4, [prepare_namespace()](https://github.com/torvalds/linux/blob/v7.4-rc4/init/do_mounts.c#L457-L480) waits for known device probes, processes root parameters, handles initrd and mounts the root filesystem.

The early-userspace documentation explains the historical initrd and initramfs paths: [early userspace support](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/early-userspace/early_userspace_support.rst).

### 7.3 rdinit= versus init=

- rdinit=/bin/sh: execute /bin/sh from the initramfs. This is useful for early boot debugging.
- init=/bin/sh: after the real root path is prepared, execute /bin/sh instead of the normal init.
- root=/dev/DEVICE: tells the kernel which block device is the real root in the normal root-mount path.
- rootwait: wait for the root device to appear; useful when device discovery is asynchronous.
- rootdelay=N: wait a fixed number of seconds before attempting the root mount.

If a valid /init runs, that program owns the early root-mount workflow. It may not use the kernel's normal prepare_namespace() path at all.

## 8. Hands-on lab: build and inspect a minimal initramfs

This lab creates a small root filesystem with BusyBox, a PID 1 script, and a compressed newc cpio archive. It validates the executable inside chroot and checks the archive. It does not build a Linux kernel.

### Prerequisites

- Linux x86-64 host for this exact BusyBox binary.
- root privileges for chroot.
- busybox, cpio, gzip and ldd.
- Optional: QEMU and a built x86 bzImage for the full boot exercise.

### Build

~~~sh
cd masterclass/lessons/00-foundations/lesson-02-linux-boot-and-early-init
sudo ./build-initramfs.sh ./initramfs.cpio.gz
~~~

The script:

1. Creates a temporary root filesystem.
2. Copies BusyBox and its shared-library dependencies.
3. installs BusyBox applet symlinks.
4. Copies /init.
5. Attempts to create /dev/console and /dev/null device nodes.
6. Runs /init --self-test inside chroot.
7. Creates a gzip-compressed newc cpio archive.
8. Validates gzip integrity and lists archive members.

Expected successful self-test output includes:

~~~text
[initramfs] self-test OK
Created: .../initramfs.cpio.gz
Archive size: ... bytes
Archive members (first 25):
...
init
bin/busybox
...
~~~

If the host/container denies CAP_MKNOD, the script reports that it could not create /dev/console or /dev/null. The archive can still be inspected, but an interactive QEMU boot needs those device nodes (or an equivalent initramfs construction that supplies them). Do not mistake a successful archive test for a successful kernel boot.

### Validate the archive manually

~~~sh
gzip -t initramfs.cpio.gz
gzip -dc initramfs.cpio.gz | cpio -it | head -30
~~~

### Optional full boot with QEMU

First build or obtain a compatible x86-64 bzImage in a disposable environment. Ensure the archive includes /dev/console and /dev/null. Then run:

~~~sh
qemu-system-x86_64 \
  -machine q35 -m 1G -smp 2 \
  -kernel /path/to/linux/arch/x86/boot/bzImage \
  -initrd ./initramfs.cpio.gz \
  -append "console=ttyS0 rdinit=/init loglevel=8 initcall_debug" \
  -nographic -no-reboot
~~~

The initramfs /init script mounts proc, sysfs and devtmpfs when supported, then starts a shell. Exit QEMU using the configured QEMU escape sequence. Do not use the host's production kernel or root filesystem for destructive experiments.

### What to inspect

- Compare the archive contents with the /init path passed by rdinit=.
- Remove /init execute permission and observe the failure.
- Rename /init and observe whether the kernel falls back to the normal root-mount path.
- Add rootwait to a test boot with a deliberately delayed root device.
- Enable initcall_debug and compare the initialization timing of built-in drivers.

## 9. Boot debugging playbook

| Symptom | First checks |
|---|---|
| No output at all | Firmware/bootloader logs, serial wiring, earlycon, image format, entry address and CPU mode |
| Output stops in decompressor | x86 compressed entry, memory placement, boot_params, relocation/KASLR and decompressor console |
| Output stops before normal console | earlycon, console=, built-in serial driver and console initialization order |
| VFS cannot mount root | root= spelling, controller/filesystem built into kernel or present in initramfs, device discovery, rootwait/rootdelay |
| No working init found | Is /init present and executable? Is its ELF interpreter and shared libraries present? Is the path correct? Check init= and rdinit= |
| A driver never probes | Is it built-in or modular? Is its firmware/module in initramfs? Are clocks/resets/regulators/IOMMU providers ready? Is probe deferred? |
| Boot stalls in one initcall | Use initcall_debug, identify the function, inspect its dependencies, wait conditions and lock ordering |
| Random memory corruption early on arm64 SoC | Verify the bootloader stopped DMA-capable devices and reserved memory correctly |

Useful command-line parameters:

- earlycon — enable an early console when supported by the platform.
- console= — select the normal kernel console.
- ignore_loglevel and loglevel=8 — expose more messages.
- initcall_debug — log initcall execution and duration.
- initcall_blacklist=FUNCTION — temporarily skip a named built-in initcall for diagnosis.
- rdinit=/bin/sh — enter early userspace shell when available.
- init=/bin/sh — use a shell as final init on the real root.
- rootwait — wait for an asynchronously discovered root device.

The kernel's [No working init found documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/admin-guide/init.rst) lists common causes and recovery guidance.

## 10. Concurrency, ordering and lifetime rules

1. **Boot CPU versus secondary CPUs:** the boot CPU initializes the core kernel first. Secondary CPU startup and full SMP scheduling occur later.
2. **Initramfs unpacking:** archive population can be asynchronous. wait_for_initramfs() is a correctness boundary before early userspace is executed.
3. **Initcall dependencies:** a consumer cannot safely assume that every provider is registered merely because its own source file is built-in. Use framework dependencies and deferred probing where appropriate.
4. **Work and init memory:** asynchronous init work must finish before free_initmem(). Do not retain callbacks into discarded sections.
5. **Console readiness:** the normal console may depend on drivers and buses that are not fully initialized when early boot messages are emitted.
6. **DMA ownership:** boot firmware must stop bus masters before Linux reuses RAM. On SoCs, this includes DSP, PCIe, USB, storage and audio DMA engines.
7. **Failure unwind:** an initcall that partially registers resources must undo them on failure; later consumers must not observe half-initialized providers.

## 11. Source archaeology: why these interfaces look this way

### x86 boot protocol history — documented fact

The x86 boot documentation explains that the protocol evolved because of the historical PC memory model, the need for the kernel image to be bootable and changing firmware/PC expectations. Protocol 2.00 introduced bzImage and initrd support with a formal bootloader handoff; later versions added relocatability, command-line and high-memory support. See the [v7.4-rc4 protocol history](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/x86/boot.rst).

### initramfs versus initrd — documented fact plus code evidence

The [initramfs buffer-format document](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/early-userspace/buffer-format.rst) states that the initramfs protocol complemented the older initial ramdisk protocol in the 2.5 development series. The archive is expanded into ramfs/rootfs rather than treated as the old block-device-style image.

A historical [2014 LZ4 decompression fix](https://github.com/torvalds/linux/commit/4d4b866aee039d609c0b40e7e5b27204607ce614) is useful archaeology: the commit explains that decompression input callbacks differ between preboot, initramfs and initrd paths. This is concrete evidence that the two mechanisms are not just interchangeable names.

### Initcall sections — observed implementation and engineering interpretation

The v7.4-rc4 linker script uses KEEP() around initcall sections so the linker does not discard the function-pointer tables. The runtime code walks those ordered ranges. The engineering interpretation is that linker-defined ordering lets independent subsystems register initialization work without placing every subsystem's initialization call in one giant central function. The levels are broad ordering contracts, not a replacement for explicit dependency management.

### Current arm64 boot fix — merged mainline, not a released 7.4 tag

[Commit 822ba3a89c7896b26e4beb70e2ebffb881924dfe](https://github.com/torvalds/linux/commit/822ba3a89c7896b26e4beb70e2ebffb881924dfe) adds an ISB after mapping the FDT in the initial identity map and includes break-before-make/barrier fixes for fixmap and KPTI updates. The maintainer pull message explains the ordering issue. This is a valuable reminder that boot-time page-table writes, TLB maintenance and instruction synchronization are separate architectural requirements. It was merged into mainline during the 7.4-rc5 development cycle; do not label it as part of a released 7.4-rc5 tag until that tag exists.

## 12. Exercises

- **Beginner:** draw the x86 and arm64 boot paths and mark where the architecture-independent start_kernel() begins.
- **Intermediate:** create two initramfs archives, one with /init and one without it. Explain the different paths selected by kernel_init_freeable().
- **Intermediate:** enable initcall_debug and identify the five slowest initcalls in a disposable test boot. Record the kernel tag and hardware/emulator.
- **Advanced:** inspect a platform driver that is required before the real root filesystem can mount. Decide whether it must be built-in or included as a module in initramfs; justify the choice.
- **Advanced:** design a bootloader-to-kernel handoff checklist for a SoC with an audio DSP, PCIe endpoint and shared-memory DMA. Include DMA quiescence, reserved-memory ownership, cache coherency and reset state.

## 13. Knowledge check and answer checkpoints

1. **Why is bzImage not just a compressed vmlinux file?**  
   It contains x86 setup/boot protocol data and decompressor code in addition to the compressed kernel payload.

2. **What does the arm64 bootloader pass in x0 for the documented FDT boot path?**  
   The physical address of the DTB.

3. **Why must DMA-capable devices be quiesced before arm64 kernel entry?**  
   A still-active bus master could overwrite RAM after the kernel begins treating it as available for its own allocations.

4. **What is the first architecture-independent C entry point?**  
   start_kernel() in init/main.c.

5. **Why does rest_init() create kthreadd before allowing init to proceed?**  
   The init task may need to create kernel threads; kthreadd must be ready first.

6. **What is the practical difference between rdinit= and init=?**  
   rdinit= selects early userspace from initramfs; init= selects the final init program after the normal root path is prepared.

7. **Why can an initramfs archive be present but /init still fail to execute?**  
   The path may be wrong or non-executable, the interpreter or shared libraries may be missing, or the binary may target an unsupported ABI.

8. **Why are initcall levels not a complete dependency system?**  
   They provide broad ordering only. Device/provider dependencies can be dynamic and require framework registration or deferred probing.

9. **Why does kernel_init() wait for asynchronous initialization before free_initmem()?**  
   An asynchronous callback may still execute code in __init sections. Reclaiming those pages too early would cause execution from freed memory.

10. **Why should a driver engineer understand the bootloader contract?**  
    Device reset, DMA ownership, reserved memory, firmware state and hardware description begin before the driver probes. Incorrect handoff can cause corruption that appears inside an unrelated driver.

## 14. References

- [Linux v7.4-rc4 source tree](https://github.com/torvalds/linux/tree/v7.4-rc4)
- [x86 boot protocol](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/x86/boot.rst)
- [arm64 boot protocol](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/arm64/booting.rst)
- [initramfs buffer format](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/early-userspace/buffer-format.rst)
- [early userspace support](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/early-userspace/early_userspace_support.rst)
- [init failure guidance](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/admin-guide/init.rst)
- [init/main.c](https://github.com/torvalds/linux/blob/v7.4-rc4/init/main.c)
- [init/initramfs.c](https://github.com/torvalds/linux/blob/v7.4-rc4/init/initramfs.c)
- [init/do_mounts.c](https://github.com/torvalds/linux/blob/v7.4-rc4/init/do_mounts.c)
- [include/linux/init.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/init.h)
- [include/asm-generic/vmlinux.lds.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/asm-generic/vmlinux.lds.h)
