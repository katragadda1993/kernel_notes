# Linux Kernel Programming Masterclass

A practical, source-driven path from foundations to advanced Linux kernel and hardware-driver engineering.

## Learning philosophy
- Understand the concept, then inspect the implementation in kernel source.
- Build and run every lab in a disposable Linux VM, QEMU, or development board.
- Explain both how an API works and why the design exists.
- Treat concurrency, object lifetime, error handling, power management, and hardware ordering as core topics.
- Never call a suspected issue a confirmed bug without reproducible evidence.

## Start here
1. [Complete curriculum](CURRICULUM.md)
2. [Lesson format and engineering standards](LESSON-STANDARD.md)

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
