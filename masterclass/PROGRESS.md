# Masterclass Progress

Last updated: 2026-11-28

## Overall status

The curriculum foundation is in place. Lessons are added sequentially under lessons/ and linked to the curriculum. A lesson is marked content-complete only when its source walkthrough, lab, debugging exercise, exercises, knowledge check and references are present.

## Lesson ledger

| Lesson | Stage | Topic | Content | Lab validation | Source baseline |
|---|---:|---|---|---|---|
| [01 — User/kernel boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md) | 0 | Privilege, syscalls, exceptions and IRQs | Complete | Built and ran on x86-64 with GCC; strace unavailable in the authoring environment; arm64 branch not cross-compiled | v7.4-rc4 |
| [02 — Linux boot and early initialization](lessons/00-foundations/lesson-02-linux-boot-and-early-init.md) | 0/1 | Firmware handoff, x86/arm64 entry, start_kernel, initcalls, initramfs and PID 1 | Complete | BusyBox initramfs built; chroot self-test and gzip/cpio checks passed. CAP_MKNOD denied device-node creation; QEMU unavailable, so full kernel boot is unverified | v7.4-rc4 |

## Coverage notes

- Stage 0 items 1–2 are introduced by Lesson 01.
- Lesson 02 covers Stage 0 boot flow and the boot/initramfs/root-filesystem topics in Stage 1.
- x86-64 and arm64 boot paths are source-reviewed at the same v7.4-rc4 tag.
- The BusyBox initramfs lab was built in the authoring container. Its /init self-test ran inside chroot, and gzip integrity plus cpio archive listing passed.
- The container denied CAP_MKNOD, so /dev/console and /dev/null could not be created. QEMU is not installed; a full kernel boot was not attempted.
- No v7.4-rc4 kernel build or arm64 boot was performed. These remain unverified.

## Next sequential lesson

**Lesson 03 — C, ELF, linker and kernel build foundations:** pointers, structs, bit operations, alignment, ELF sections/symbols/relocations, calling conventions, Kconfig/Kbuild and safe kernel build setup.

## Definition of done

- Lesson and code are committed to main.
- Source links are pinned to a kernel tag or commit.
- Lab commands and expected output are documented.
- Any execution that was not performed is explicitly marked unverified.
- At least 10 non-duplicate interview questions are added for every full lesson.
- Upstream status and trend checkpoints are dated and distinguish merged code from proposals.
