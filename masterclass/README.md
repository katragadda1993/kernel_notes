# Linux Kernel Programming Masterclass

A practical, source-driven path from foundations to advanced Linux kernel and hardware-driver engineering.

## Current progress
Lessons 01–04 are content-complete. The syscall and ELF labs ran on x86-64; the BusyBox initramfs and chroot self-test passed; an external module compiled against 6.12.107 headers but was not loaded into the 6.18.57 host. Sparse, arm64 execution and a full QEMU kernel boot remain unverified. See [PROGRESS.md](PROGRESS.md).

## Learning philosophy
- Understand the concept, then inspect the implementation in kernel source.
- Build and run every lab in a disposable Linux VM, QEMU, or development board.
- Explain both how an API works and why the design exists.
- Treat concurrency, object lifetime, error handling, power management, and hardware ordering as core topics.
- Never call a suspected issue a confirmed bug without reproducible evidence.

## Start here
1. [Complete curriculum](CURRICULUM.md)
2. [Lesson format and engineering standards](LESSON-STANDARD.md)
3. [Linux kernel and driver interview question bank](INTERVIEW-QUESTION-BANK.md) — 166 source-backed questions, continuously expanding.
4. [Progress ledger](PROGRESS.md) — lesson completion and validation status.
5. [Lesson 01: User/kernel boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md) — privilege, syscalls, exceptions and IRQs.
6. [Lesson 02: Linux boot and early initialization](lessons/00-foundations/lesson-02-linux-boot-and-early-init.md) — x86/arm64 entry, initcalls, initramfs and PID 1.
7. [Lesson 03: C, ELF, linker and Kbuild](lessons/00-foundations/lesson-03-c-elf-kernel-build.md) — kernel C types, ELF relocations, linker scripts and module build.
8. [Lesson 04: Source tree, configuration and reproducible builds](lessons/00-foundations/lesson-04-kernel-tree-config-reproducible-builds.md) — MAINTAINERS, Kconfig, fragments, O= trees and toolchains.
9. [Upstream merge and trend watch](UPSTREAM-TREND-WATCH.md) — mainline, linux-next, merge windows, subsystem trends and weekly engineering analysis.
10. [Driver engineering track](DRIVER-ENGINEERING-TRACK.md) — end-to-end hardware-driver lifecycle and capstone projects.
11. [Kernel source archaeology](SOURCE-ARCHAEOLOGY.md) — learn why designs exist through commits, reviews and mailing-list history.

## Curriculum map
| Stage | Focus |
|---|---|
| 0 | Systems and C foundations |
| 1 | Kernel architecture, source tree, build and boot |
| 2 | Kernel C, modules, logging and debugging |
| 3 | Memory, processes, scheduling and synchronization |
| 4 | Kernel interfaces and driver fundamentals |
| 5 | Platform drivers, Device Tree, MMIO and interrupts |
| 6 | DMA, PCIe, USB, I2C, SPI and I3C |
| 7 | Clocks, resets, pinctrl, regulators and power management |
| 8 | ALSA, ASoC, SoundWire, HDA and DSP-facing drivers |
| 9 | Networking, storage, IOMMU and graphics |
| 10 | Tracing, sanitizers, static analysis and testing |
| 11 | Performance, reliability, security and power optimization |
| 12 | Upstream contribution and patch review |
| 13 | Architecture-specific internals, advanced memory and boot internals |
| 14 | Firmware, heterogeneous SoCs, remoteproc, RPMsg, mailbox and SCMI |
| 15 | eBPF, observability and programmable scheduling (sched_ext) |
| 16 | Virtualization, KVM, virtio, VFIO and SR-IOV |
| 17 | Security, VFS/filesystems and advanced storage/networking |
| 18 | Rust for Linux, modern kernel engineering and upstream trend analysis |

## Learning paths
1. Linux foundations and kernel internals
2. Device-driver engineering
3. Hardware and SoC integration
4. Debugging, observability and performance
5. Security, virtualization and systems subsystems
6. Kernel architecture, upstream engineering and source archaeology

## Target repository structure
- labs/: progressively harder, runnable examples
- notes/: architecture and source walkthroughs
- exercises/: debugging tasks and assignments
- references/: kernel documentation and specifications

## Environment policy
Use a supported LTS kernel for repeatable labs. Record the exact kernel release, commit, compiler, architecture, configuration and hardware used. Use a VM or non-production board for experiments. Never load untrusted modules on a production machine.

## Completion criteria
Every lesson should include working code or a source walkthrough, build/run instructions, expected output, failure modes, debugging steps, exercises, a knowledge check and references.

## License
The repository's existing top-level license governs contributions unless a file states otherwise. Preserve notices and license terms when referencing or adapting upstream code.
