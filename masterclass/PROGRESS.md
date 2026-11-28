# Masterclass Progress

Last updated: 2026-11-28

## Overall status

The foundation lessons now cover privilege/syscall entry, boot/initramfs, C/ELF/Kbuild fundamentals, source-tree/configuration/build workflow, and the module runtime/logging/teardown lifecycle. A lesson is marked content-complete only when its source walkthrough, lab, debugging exercise, exercises, knowledge check and references are present. Lab validation is reported separately from content completeness.

## Lesson ledger

| Lesson | Stage | Topic | Content | Lab validation | Source baseline |
|---|---:|---|---|---|---|
| [01 — User/kernel boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md) | 0 | Privilege, syscalls, exceptions and IRQs | Complete | Built and ran on x86-64 with GCC; strace unavailable in the authoring environment; arm64 branch not cross-compiled | v7.4-rc4 |
| [02 — Linux boot and early initialization](lessons/00-foundations/lesson-02-linux-boot-and-early-init.md) | 0/1 | Firmware handoff, x86/arm64 entry, start_kernel, initcalls, initramfs and PID 1 | Complete | bash -n passed; BusyBox initramfs built; chroot self-test and gzip/cpio checks passed. CAP_MKNOD denied device-node creation; QEMU unavailable, so full kernel boot is unverified | v7.4-rc4 |
| [03 — C, ELF, linker and Kbuild](lessons/00-foundations/lesson-03-c-elf-kernel-build.md) | 0/1 | Kernel C types/annotations, layout, ELF sections/symbols/relocations, linker scripts, Kconfig/Kbuild and module build | Complete | x86-64 userspace lab built/run at -O0/-O2; readelf/objdump/nm inspected; external .ko built against 6.12.107 headers. Not loaded because host is 6.18.57; Sparse, full v7.4-rc4 build and arm64 build unverified | v7.4-rc4 |
| [04 — Source tree, configuration and reproducible builds](lessons/00-foundations/lesson-04-kernel-tree-config-reproducible-builds.md) | 1 | Source tree, MAINTAINERS, Kconfig, fragments, Kbuild outputs, architecture/toolchain selection and build provenance | Complete | bash -n passed; audit helper control flow tested against a minimal mock fixture. This is not a real Linux source-tree audit. Full v7.4-rc4 Kconfig merge/build, arm64 cross-build and QEMU boot not performed | v7.4-rc4 |
| [05 — Kernel runtime, modules, logging and safe teardown](lessons/00-foundations/lesson-05-kernel-runtime-modules-logging-debugging.md) | 2 | Module lifecycle, parameters, printk/pr_*/dev_*, dynamic debug, delayed work, error unwinding and safe unload | Complete | External module built against 6.12.107+deb13-amd64 headers; modinfo and readelf inspected. Not loaded because host runs 6.18.57. Runtime workqueue, dynamic debug, init-failure injection, Sparse, v7.4-rc4 build, arm64 and hardware remain unverified | v7.4-rc4 |

## Coverage notes

- Stage 0 items 1–2 are introduced by Lesson 01.
- Lesson 02 covers Stage 0 boot flow and the boot/initramfs/root-filesystem topics in Stage 1.
- x86-64 and arm64 boot paths are source-reviewed at the same v7.4-rc4 tag.
- Lesson 03's userspace ELF lab was compiled and executed at -O0 and -O2 with GCC 14.2.0; both produced the expected output. readelf, objdump and nm inspection confirmed the ELF64 relocatable object, custom section, symbols and relocations.
- The external hello module compiled against Debian kernel headers 6.12.107+deb13-amd64. The running host kernel was 6.18.57, so the .ko was inspected but intentionally not loaded. This is not a v7.4-rc4 kernel build.
- The BusyBox initramfs lab was built in the authoring container. Its /init self-test ran inside chroot, and gzip integrity plus cpio archive listing passed.
- The container denied CAP_MKNOD, so /dev/console and /dev/null could not be created. QEMU is not installed; a full kernel boot was not attempted.
- Lesson 04's audit helper passed bash -n and was executed against a deliberately minimal mock source/output fixture. It reported the fixture's mock version/config as expected. The fixture does not validate actual kernel Kconfig/Kbuild behavior.
- The authoring environment has kernel headers (6.12.107/6.12.63) but no full kernel source checkout. No real v7.4-rc4 defconfig/fragment merge, full kernel build, arm64 cross-build or QEMU boot was performed.
- Lesson 05's external module built successfully with Kbuild against /usr/src/linux-headers-6.12.107+deb13-amd64. modinfo showed all three parameter definitions and the 6.12.107 vermagic. readelf showed .init.text, .exit.text, .modinfo and __param. The running host is 6.18.57, so insertion/removal was deliberately not attempted. Runtime workqueue behavior, dynamic-debug control and injected init failure remain unverified.
- Sparse is not installed. No full v7.4-rc4 build, arm64 cross-build, QEMU boot or hardware validation is claimed.

## Next sequential lesson

**Lesson 06 — Kernel Memory Allocation and GFP Context:** kmalloc/kzalloc/kcalloc/kfree, vmalloc/vfree, GFP flags, allocation context, failure handling, slab/page allocator overview and memory-lifetime debugging.

## Definition of done

- Lesson and code are committed to main.
- Source links are pinned to a kernel tag or commit.
- Lab commands and expected output are documented.
- Any execution that was not performed is explicitly marked unverified.
- At least 10 non-duplicate interview questions are added for every full lesson.
- Upstream status and trend checkpoints are dated and distinguish merged code from proposals.


## Lesson 05 runtime validation detail

- Build command: make KDIR=/usr/src/linux-headers-6.12.107+deb13-amd64 clean all.
- Result: Kbuild compile, modpost and link succeeded. modinfo confirmed lesson05_runtime, version 1.0, parameter descriptions and vermagic 6.12.107+deb13-amd64.
- readelf -S confirmed .init.text, .exit.text, .modinfo and __param sections.
- The host kernel is 6.18.57, so the module was not loaded. No runtime log, dynamic-debug, workqueue cancellation or injected-init-failure result is claimed.
