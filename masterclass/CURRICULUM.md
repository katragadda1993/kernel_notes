# Complete Curriculum: Linux Kernel Programming from Basics to Advanced

Each module should contain a lesson, kernel-source walkthrough, lab, debugging exercise and review questions. Track completion in [PROGRESS.md](PROGRESS.md).

## Stage 0 — Systems foundations
1. OS responsibilities; user mode vs kernel mode
2. CPU privilege levels, traps, exceptions, interrupts and system calls
3. Boot flow: firmware, bootloader, kernel, initramfs and userspace
4. x86-64 and ARM64 architecture basics
5. C refresh: pointers, structs, unions, bit operations and undefined behavior
6. ELF, linker, symbols, sections, relocation and calling conventions
7. Endianness, alignment, cache lines, barriers and atomics
8. Shell, Git, Make, GCC/Clang, GDB and cross-compilation
Lab: inspect an ELF binary and trace a system call.

**Completed lesson:** [Lesson 01 — The User/Kernel Boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md) covers OS protection, CPU privilege states, system calls, exceptions and interrupts. Its x86-64 lab was built and run; arm64 and strace validation remain outstanding.

## Stage 1 — Kernel architecture and environment
1. Kernel source tree and subsystem ownership
2. Mainline, stable, LTS and vendor kernels
3. Kconfig, Makefiles, configuration fragments and generated headers
4. Configure, build, install and boot a kernel safely
5. Kernel command line, initramfs and root filesystem
6. Coding style, documentation and source navigation
7. Out-of-tree modules and module lifecycle
8. printk, dmesg and dynamic debug
Lab: build and boot a kernel in QEMU; load a hello module.

## Stage 2 — Kernel C and essential APIs
1. Kernel coding conventions and GNU C
2. Kernel types: size_t, phys_addr_t, dma_addr_t and __iomem
3. Allocation: kmalloc, kzalloc, kcalloc, vmalloc and devm
4. Error pointers, errno and cleanup patterns
5. Lists, rbtrees, xarrays, IDR and IDA
6. refcount_t, kref and object lifetime
7. Completions, wait queues and poll
8. Workqueues, timers and delayed work
9. Per-CPU data and CPU hotplug awareness
Lab: build a module with managed resources, asynchronous work and robust error unwinding.

## Stage 3 — Core kernel internals
1. Process creation, fork/exec and task_struct
2. Scheduler classes, run queues and context switching
3. Virtual memory, page tables and page faults
4. Page allocator, slab allocators, GFP flags and reclaim
5. VFS: file, inode, dentry and mount concepts
6. System calls and safe user/kernel data transfer
7. Hard IRQ, softirq and threaded IRQ contexts
8. Preemption, RCU, seqlocks and read/write synchronization
9. Lock ordering, deadlocks and lockdep
10. Races, use-after-free and reference leaks
Lab: trace a system call and diagnose a deliberately introduced race.

## Stage 4 — Driver model and user interfaces
1. Linux device model: device, driver, bus and class
2. Character devices and major/minor numbers
3. file_operations: open, read, write, poll, ioctl, mmap and release
4. sysfs, procfs, debugfs and configfs
5. udev, devtmpfs and device-node lifecycle
6. Platform devices and platform drivers
7. Device Tree: nodes, properties, phandles and bindings
8. ACPI enumeration and firmware-described devices
9. devres, probe ordering and deferred probe
10. remove, shutdown and hot-unplug safety
Lab: create a character driver and a simple platform driver.

## Stage 5 — Register-level hardware programming
1. MMIO vs port I/O; map and unmap registers
2. readl/writel, relaxed accessors and endianness
3. Register fields, reserved bits and read-modify-write hazards
4. Posted writes, readback and ordering
5. Interrupt status/ack registers and interrupt storms
6. Shared IRQs, threaded IRQs, MSI and MSI-X
7. Polling, timeout and recovery design
8. Reset sequencing and device readiness
9. Hardware state machines and safe teardown
10. regmap and register-level debug
Lab: implement a mock register-backed driver and document its register sequence.

## Stage 6 — DMA and high-speed interconnects
1. CPU virtual, physical and device DMA addresses
2. DMA masks; coherent vs streaming mappings
3. dma_alloc_coherent, dma_map_* and dma_sync_* APIs
4. Scatter-gather lists, descriptor rings and ownership
5. Cache coherency, barriers, descriptor ordering and doorbells
6. DMAengine provider/client architecture
7. Cyclic DMA, completion and residue
8. PCIe enumeration, config space, BARs and resources
9. PCIe DMA, MSI/MSI-X, reset, AER and hotplug
10. IOMMU, IOVA and DMA isolation
Lab: implement a DMAengine client and trace descriptor ownership end to end.

## Stage 7 — Peripheral buses
1. I2C adapter/client model, transfers, retries and errors
2. SPI controller/device model, modes, chip select and queues
3. I3C model, dynamic address assignment and transfers
4. GPIO descriptors, pinctrl and interrupt mapping
5. regmap, regcache and volatile registers
6. Firmware nodes and common device properties
7. Bus recovery, timeouts and suspend/resume
8. Enumeration and device removal
Lab: write a mock I2C or SPI client and exercise error paths.

## Stage 8 — Power and platform integration
1. Common Clock Framework: providers, consumers and rates
2. Reset controller framework and reset ordering
3. Regulators and voltage sequencing
4. Runtime PM, usage counts and autosuspend
5. System suspend/resume and wakeup sources
6. devfreq, OPP tables and performance states
7. Thermal framework and cooling devices
8. Interconnect bandwidth requests
9. Pin states and low-power states
10. Probe dependencies and power-domain sequencing
Lab: add runtime PM to a driver and test repeated suspend/resume.

## Stage 9 — Audio and DSP driver engineering
1. ALSA architecture: cards, PCM, controls and components
2. PCM lifecycle: hw_params, prepare, trigger, pointer and mmap
3. Periods, buffers, constraints, timestamps and XRUNs
4. ASoC machine, codec, CPU DAI and component drivers
5. DAPM widgets, routes and power sequencing
6. DMA-backed audio and cyclic transfers
7. SoundWire enumeration, banks, ports and stream lifecycle
8. HD Audio controller, CORB/RIRB, stream descriptors and BDL
9. I2S/TDM framing, clocks, slots and codec integration
10. DSP firmware loading, IPC/mailbox and memory windows
11. Audio PM, clocking, latency and glitch debugging
12. Audio tracepoints, loopback tests and end-to-end validation
Lab: trace an ALSA PCM stream from userspace through DMA to hardware.

## Stage 10 — Networking, storage, USB and graphics
1. Network device model, NAPI, SKB and DMA rings
2. Ethernet MAC/PHY, phylink, MDIO and offloads
3. USB architecture, URBs, endpoints and xHCI
4. Block layer, requests, bios and blk-mq
5. NVMe queues, doorbells, interrupts and timeout recovery
6. SCSI and storage error handling
7. DRM/KMS, GEM/TTM and display pipelines
8. GPU memory, fences, DMA-BUF and synchronization
9. Device reset, firmware loading and recovery
10. Performance counters and subsystem tracing
Lab: trace one packet, USB transfer, block request or display commit.

## Stage 11 — Testing, debugging, security and performance
1. KUnit and test fixtures
2. kselftest and subsystem test suites
3. Sparse, Smatch, Coccinelle and compiler diagnostics
4. KASAN, KCSAN, UBSAN, KFENCE and kmemleak
5. lockdep, DEBUG_ATOMIC_SLEEP and fault injection
6. ftrace, trace-cmd, perf and function-graph tracing
7. Crash dumps, vmcore, crash utility and pstore
8. syzkaller concepts and reproducer minimization
9. Capabilities, namespaces and LSM overview
10. Input validation, ioctl design and DMA isolation
11. Latency, throughput, CPU and power profiling
12. Regression testing and git bisect
Lab: reproduce and fix a race or error-path bug with a regression test.

## Stage 12 — Upstream-quality engineering
1. MAINTAINERS, subsystem docs and contribution guidelines
2. Find maintainers and inspect recent commit history
3. Minimal patches and logical patch series
4. Commit messages, Signed-off-by and patch formatting
5. checkpatch, builds and subsystem tests
6. Review lifetime, locking, ABI and error-path correctness
7. Submit patches using the subsystem's accepted workflow
8. Respond to review and revise patches
9. Stable backports, Fixes tags and regression handling
10. Capstone: implement, test, document and submit a driver improvement

## Recommended path for hardware engineers
Prioritize Stages 0–6, then Stage 8 and Stage 9. Continue through other subsystems while practicing upstream contribution. Do not skip synchronization, memory lifetime, DMA semantics or error unwinding.

## Lesson cadence
Break stages into focused 45–90 minute lessons. A lesson is complete only when its lab, expected output, failure cases and knowledge check are present. Link to exact upstream source revisions rather than copying large source files.


## Stage 13 — Architecture, boot and advanced memory
1. x86-64 and ARM64 exception entry, privilege levels and context switching
2. SMP boot, CPU hotplug, topology, NUMA and cache locality
3. TLB shootdowns, page-table locking and memory barriers
4. Buddy allocator, SLUB internals, reclaim, compaction and huge pages
5. Memory pressure, OOM, page migration and working-set behavior
6. Early boot, initcalls, linker sections and initramfs handoff
7. Boot-time failures, early console and panic triage
Lab: trace a page fault and allocation path on a pinned kernel revision.

## Stage 14 — Firmware and heterogeneous SoC integration
1. remoteproc lifecycle, firmware loading and crash recovery
2. RPMsg endpoints, virtio transport and shared-memory vrings
3. Mailbox controller framework and interrupt-driven IPC
4. SCMI protocols, transports and firmware-owned resources
5. Shared memory, cache maintenance, IOMMU and DSP/accelerator access
6. Power domains, interconnect bandwidth and device dependencies
7. Secure firmware loading, authentication and version compatibility
8. Debugging host/firmware ownership, timeouts and reset races
Lab: build a host-side mock or QEMU-based IPC exercise; document what requires real hardware.

## Stage 15 — eBPF and modern observability
1. BPF architecture, bytecode, maps, helpers and program types
2. Verifier model, BTF, CO-RE and kfuncs
3. Tracepoints, fentry/fexit, kprobes, uprobes and perf events
4. bpftrace and libbpf-based tooling
5. sched_ext architecture and BPF-defined scheduling policy
6. BPF performance, safety boundaries and production rollout
Lab: instrument a driver or kernel subsystem without modifying its source.

## Stage 16 — Virtualization and device assignment
1. KVM, VM exits, vCPU lifecycle and virtual interrupts
2. virtio, vhost and virtqueue descriptor ownership
3. VFIO, IOMMU groups and device assignment
4. SR-IOV, PF/VF lifecycle and isolation
5. Guest/host memory mapping and DMA translation
6. Virtualized device reset, migration and error recovery
Lab: trace a virtio or VFIO data path in a VM.

## Stage 17 — VFS, filesystems, storage and advanced networking
1. VFS operation tables, inode/dentry lifetime and mount namespaces
2. Page cache, writeback, direct I/O and filesystem consistency
3. Block multiqueue, blk-mq scheduling and NVMe queue lifecycle
4. TCP/IP receive/transmit paths, NAPI and SKB lifetime
5. XDP, AF_XDP, zero-copy and NIC queue scaling
6. RSS, RPS/RFS, IRQ affinity and NUMA-aware networking
7. Lock contention, batching and high-throughput networking
Lab: trace one I/O or packet end-to-end and measure the bottleneck.

## Stage 18 — Rust for Linux and current kernel engineering
1. Rust-for-Linux architecture, bindings and safe abstractions
2. Driver support boundaries, pinning, ownership and FFI
3. C/Rust interoperability and review expectations
4. Stable API reality, subsystem interfaces and backport constraints
5. Kernel CI, regression tracking and bisect workflows
6. AI-assisted patch generation/review: evidence, validation and maintainer expectations
7. Maintainer workflows, patch series quality and long-term maintenance
Lab: inspect a merged Rust or core-kernel change and compare its C and Rust boundaries.

## Cross-cutting capstone projects
- Character driver with robust lifetime and concurrent access
- Platform/Device Tree driver with MMIO, IRQ and runtime PM
- DMAengine client with descriptor ownership and coherency tests
- PCIe device lifecycle and AER/reset recovery analysis
- Mock I2C/SPI driver with fault injection
- ALSA PCM driver/path analysis with XRUN and latency diagnosis
- remoteproc/RPMsg mailbox IPC design review
- Driver teardown race and use-after-free investigation
- Kernel performance regression bisect and trace-based root cause
- Small upstream-quality patch with tests and review response

## Trend-driven learning rule
The curriculum is a stable foundation, not a frozen syllabus. Each run must inspect current mainline and linux-next activity, identify relevant merged changes, and add focused lessons when the change materially affects kernel architecture, APIs, drivers, performance, security or engineering practice. Do not turn every commit into a lesson; prioritize impact and explain why.
