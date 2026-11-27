# Masterclass Progress

Last updated: 2026-11-28

## Overall status

The curriculum foundation is in place. Lessons are added sequentially under lessons/ and linked to the curriculum. A lesson is marked content-complete only when its source walkthrough, lab, debugging exercise, exercises, knowledge check and references are present.

## Lesson ledger

| Lesson | Stage | Topic | Content | Lab validation | Source baseline |
|---|---:|---|---|---|---|
| [01 — User/kernel boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md) | 0 | Privilege, syscalls, exceptions and IRQs | Complete | Built and ran on x86-64 with GCC; strace unavailable in the authoring environment; arm64 branch not cross-compiled | v7.4-rc4 |

## Coverage notes

- Stage 0 items 1–2 are introduced by Lesson 01.
- The lesson compares native x86-64 and arm64 entry paths.
- The first lab is a userspace program; no kernel module or device is modified.
- The raw x86-64 syscall path was compiled and run in the authoring environment.
- The arm64 SVC path is source-reviewed but requires an arm64 build/run for validation.
- strace instructions are included but were not executed because strace was not installed in the authoring environment.

## Next sequential lesson

**Lesson 02 — Linux boot sequence and early initialization:** firmware/UEFI or platform firmware, bootloader, architecture entry, decompression/relocation, early kernel initialization, initcalls, initramfs and the first userspace process.

## Definition of done

- Lesson and code are committed to main.
- Source links are pinned to a kernel tag or commit.
- Lab commands and expected output are documented.
- Any execution that was not performed is explicitly marked unverified.
- At least 10 non-duplicate interview questions are added for every full lesson.
- Upstream status and trend checkpoints are dated and distinguish merged code from proposals.
