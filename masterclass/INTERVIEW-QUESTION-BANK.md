# Linux Kernel and Device Driver Interview Question Bank

A growing practice bank from beginner to advanced. For each question, practise a concise explanation, then a deeper answer using architecture diagrams, kernel source references, trade-offs and debugging examples.

## How to answer driver questions
Always cover execution context, ownership and lifetime, synchronization, error paths, hardware ordering, power management, and how you would validate the behavior.

## 1. Linux and kernel fundamentals

1. What is the difference between user space and kernel space?
2. Describe the boot sequence from reset to the first userspace process.
3. What happens inside the kernel when a process calls read()?
4. What is a system call, and how does it differ from a hardware interrupt?
5. Which Linux interfaces are stable ABI, and are internal kernel APIs stable?
6. Compare built-in drivers and loadable modules.
7. What are initcalls, and why does initialization ordering matter?
8. What are Kconfig and Kbuild responsible for?

## 2. Kernel C, memory and lifetime

9. Explain GFP_KERNEL versus GFP_ATOMIC. Which contexts can sleep?
10. Compare kmalloc(), vmalloc() and page allocation.
11. Why is volatile not a synchronization primitive?
12. Explain virtual, physical and DMA addresses. Why may they differ?
13. What are ERR_PTR(), IS_ERR() and PTR_ERR()?
14. What is devm resource management? When is explicit cleanup still needed?
15. Explain kref and refcount_t. How do they protect object lifetime?
16. Give examples of use-after-free, double-free and memory leak in drivers.
17. Why must user pointers be accessed with copy_to_user()/copy_from_user()?
18. What do __user and __iomem annotations help detect?

## 3. Processes, scheduling and synchronization

19. What is task_struct?
20. Compare process context, softirq context and hard IRQ context.
21. Why can a mutex not be acquired in hard IRQ context?
22. Compare mutexes, spinlocks, rwlocks and semaphores.
23. Compare spin_lock_irqsave() and spin_lock_bh().
24. Give an example of a race between an interrupt handler and a worker thread.
25. How do lock ordering and lockdep help prevent deadlocks?
26. Explain wait queues versus completions.
27. Why use workqueues to defer interrupt processing?
28. What is RCU, and where is it useful?
29. What is a memory barrier? Why might descriptor writes need ordering before a device doorbell?
30. What are atomic operations, and what problems do they not solve?
31. What changes in driver assumptions can PREEMPT_RT introduce?

## 4. Device model and driver frameworks

32. Explain the Linux device model: bus, device, driver and class.
33. Walk through a platform driver's probe() sequence.
34. What causes -EPROBE_DEFER, and how should it be handled?
35. Compare platform, PCI and USB drivers.
36. How does a driver match a Device Tree node?
37. What is a Device Tree binding, and why is schema validation important?
38. Compare sysfs, procfs and debugfs.
39. When should an interface use ioctl, sysfs or an existing subsystem API?
40. Explain cdev, major/minor numbers and file_operations.
41. What must a remove() path do before freeing resources?
42. How do you handle hot-unplug when userspace still has an open file descriptor?
43. What is regmap, and when is it useful?
44. What is deferred probing, and what dependency cycles can it expose?

## 5. MMIO, registers and interrupts

45. What is MMIO, and why should a driver map resources using kernel I/O mapping helpers?
46. Why use readl()/writel() instead of ordinary pointer dereferences?
47. Compare readl() and readl_relaxed().
48. Explain posted writes and when readback may be needed.
49. What is a read-modify-write race on a hardware register?
50. How do write-one-to-clear status bits affect register handling?
51. Compare polling, hard IRQ and threaded IRQ designs.
52. What work belongs in a top-half interrupt handler?
53. Why must an interrupt source be acknowledged or masked correctly?
54. What are shared IRQ requirements?
55. Compare MSI/MSI-X with legacy INTx.
56. How do you safely free an IRQ while a device is being removed?
57. How would you debug an interrupt storm or missing interrupt?

## 6. DMA and DMAengine

58. Why is dma_addr_t not necessarily a physical address?
59. Compare coherent DMA allocations and streaming DMA mappings.
60. Explain dma_map_single(), dma_unmap_single() and DMA direction.
61. What do DMA_TO_DEVICE, DMA_FROM_DEVICE and DMA_BIDIRECTIONAL mean?
62. Why must dma_mapping_error() be checked?
63. What are DMA masks, and why configure them?
64. Explain cache coherency on non-coherent systems.
65. When are dma_sync_single_for_cpu() and dma_sync_single_for_device() required?
66. Explain scatter-gather DMA and the mapped-entry count.
67. What is a descriptor ring, and how does ownership move between CPU and device?
68. Why may a memory barrier be required before a DMA doorbell write?
69. Compare DMAengine provider and client drivers.
70. What are slave DMA and cyclic DMA? Give an audio example.
71. How do you terminate DMA safely during timeout or driver removal?
72. How would you debug DMA corruption that occurs only under load?
73. What role does an IOMMU play in DMA translation and isolation?

## 7. PCIe, USB and peripheral buses

74. Describe PCIe enumeration and driver binding.
75. What are BARs, and how does a driver request and map one?
76. Compare PCIe config space and device MMIO registers.
77. How are MSI/MSI-X vectors allocated and released?
78. What is PCIe AER, and how should a driver recover?
79. Compare FLR and a bus-level reset.
80. What is a USB URB? Explain submission and completion.
81. Compare USB control, bulk, interrupt and isochronous transfers.
82. Explain I2C adapter/client architecture and repeated-start transfers.
83. Compare SPI controller and SPI device drivers.
84. What distinguishes I3C from I2C at the bus-management level?
85. How should a bus driver handle timeout, NACK and controller recovery?

## 8. Clocks, resets and power management

86. Compare runtime PM and system suspend/resume.
87. What is a runtime PM usage counter, and how can it become unbalanced?
88. Why must clocks, regulators and resets be sequenced?
89. What can happen if a driver accesses MMIO while its power domain is off?
90. Explain autosuspend and bugs involving in-flight I/O.
91. How should IRQ wakeup be configured for system suspend?
92. How would you debug a device that works after cold boot but fails after resume?

## 9. ALSA, ASoC and audio drivers

93. Explain the ALSA PCM lifecycle from open to close.
94. What are PCM periods, buffer size and period size?
95. Explain hw_params(), prepare(), trigger() and pointer().
96. What causes an XRUN? Compare underrun and overrun.
97. Explain ASoC machine, CPU DAI, codec and platform components.
98. What is DAPM, and how does it manage audio power paths?
99. How does cyclic DMA support continuous audio?
100. How would you debug audio glitches that occur only at 96 kHz?
101. Explain SoundWire enumeration, banks, ports and stream lifecycle.
102. Compare SoundWire command/response handling with HD Audio CORB/RIRB.
103. Explain HD Audio BDL descriptors and stream position tracking.
104. How do BCLK, frame sync, slot width and sample rate interact in I2S/TDM?
105. How would you trace a PCM period from userspace through DMA to the codec?

## 10. Debugging, testing and upstream contribution

106. When would you use ftrace, tracepoints, dynamic debug and perf?
107. Compare KASAN, KCSAN, UBSAN, KFENCE and kmemleak.
108. What does Sparse detect? What does Smatch add?
109. Compare KUnit and kselftest.
110. How would you build a minimal reproducer for an intermittent driver hang?
111. Explain git bisect and reliable good/bad test criteria.
112. What is fault injection, and how can it expose error-path bugs?
113. What belongs in a high-quality kernel bug report?
114. What does checkpatch.pl check, and why is passing it not proof of correctness?
115. Explain MAINTAINERS, Fixes tags and Signed-off-by.
116. Describe the process of preparing and submitting an upstream kernel patch.

## 11. Scenario-based driver debugging

117. DMA completes, but the CPU reads stale data. What do you inspect?
   - DMA direction, mapping lifetime, sync calls, cache coherency, ownership transitions and barriers.
118. A driver works on x86 but fails on ARM64. Which assumptions do you challenge?
   - Address identity, coherency, alignment, endianness, ordering, DMA masks and MMIO semantics.
119. An interrupt arrives after remove() starts. How do you make teardown safe?
   - Stop hardware generation, mask source, synchronize IRQ, cancel work, terminate DMA and release resources in safe order.
120. Audio works at 48 kHz but glitches at 96 kHz. What do you measure?
   - Clocking, bandwidth, FIFO thresholds, period deadlines, DMA cadence, interrupt latency and bus contention.
121. Probe succeeds only on the second boot. What do you investigate?
   - Dependency readiness, reset/clock/regulator order, firmware state, deferred probe and stale hardware state.
122. A PCIe device disappears under load. How do you debug it?
   - AER logs, link state, PM, reset behavior, DMA/IOMMU faults, interrupt handling and recovery.
123. A warning appears only with lock debugging enabled. What does that suggest?
   - Locking contract violation, lock ordering or context misuse; inspect lockdep stacks and reproduce.
124. Memory leaks only when probe fails halfway through. How do you find the leak?
   - Enumerate acquisitions, inspect every failure edge, devres boundaries, references and fault injection.
125. The system hangs during suspend while a device is active. What ordering do you inspect?
   - Quiesce I/O, stop DMA, drain work, mask/synchronize IRQ, PM state, clocks, resets and wakeup.
126. A register read returns all ones. What possible causes do you test?
   - Device absent/unpowered/reset, wrong resource, bus error, access width, mapping or link failure.

## Interview answer framework

For each driver design question, answer these:
- Which context executes this code, and can it sleep?
- Who owns each object/resource, and when is it released?
- Which lock protects shared state?
- What happens on timeout, partial failure and device removal?
- What ordering does the hardware require?
- How do DMA mapping and cache synchronization work on this architecture?
- What happens during runtime suspend, system suspend and reset?
- How would you reproduce, instrument and validate the issue?

## Expansion rule for future lessons

Every new lesson should add at least 10 related interview questions: 4 fundamentals, 3 implementation/source questions, 2 debugging scenarios and 1 design/trade-off question. Include concise answer guidance and link the questions to the relevant lesson.

## 12. Lesson 01 — User/kernel boundary, system calls and entry paths (Questions 127–136)

**127. [Beginner | Fundamentals] What enforces the user/kernel privilege boundary?**  
Answer checkpoint: CPU privilege state, MMU/page-table permissions and controlled exception-entry mechanisms enforce it. A C convention alone is not a security boundary.

**128. [Beginner | Fundamentals] How does a system call differ from a hardware interrupt?**  
Answer checkpoint: A syscall is a deliberate synchronous request from the running program. A hardware IRQ is asynchronous and is raised by a timer or device. They have different entry causes and handling paths.

**129. [Intermediate | Fundamentals] What are the native x86-64 Linux syscall argument registers?**  
Answer checkpoint: RAX holds the number; RDI, RSI, RDX, R10, R8 and R9 hold arguments 0–5. RCX and R11 are used by SYSCALL for return RIP and flags.

**130. [Intermediate | Fundamentals] What are the native arm64 Linux syscall registers?**  
Answer checkpoint: X8 holds the syscall number; X0–X5 hold arguments; X0 carries the return value. SVC #0 causes the EL0→EL1 synchronous exception.

**131. [Intermediate | Source/API] Why does x86-64 entry assembly switch stacks even though SYSCALL has already entered kernel privilege?**  
Answer checkpoint: SYSCALL does not change RSP. Linux must save the user stack pointer and select a trusted per-task kernel stack before using the stack for kernel frames and C calls.

**132. [Advanced | Source/API] In v7.4-rc4, why is it inaccurate to say x86-64 dispatch always indexes sys_call_table[]?**  
Answer checkpoint: arch/x86/entry/syscall_64.c documents that the array is retained for tracing metadata; x64_sys_call() dispatches through a switch generated from syscall definitions.

**133. [Intermediate | Source/API] How does arm64 distinguish SVC from a page fault in the EL0 synchronous handler?**  
Answer checkpoint: el0t_64_sync_handler() reads ESR_EL1, decodes the exception class and routes SVC64 to el0_svc(), while abort classes use data/instruction-abort handlers.

**134. [Intermediate | Debugging] clock_gettime() is absent from strace. Does that prove the program did not read the clock?**  
Answer checkpoint: No. The vDSO can serve clock_gettime in userspace. Compare with an explicit syscall(SYS_clock_gettime, ...), and inspect the process ABI and tracing configuration.

**135. [Advanced | Debugging] A syscall returns ENOSYS on one target but works on another. What do you inspect first?**  
Answer checkpoint: Running architecture and ABI, syscall number/table for that kernel revision, compatibility mode, kernel configuration and seccomp filters. ENOSYS alone does not prove the feature is absent.

**136. [Advanced | Architecture/trade-off] Why does x86-64 use SYSRET only when the saved state satisfies strict conditions and otherwise fall back to IRET?**  
Answer checkpoint: SYSRET is a fast path with narrower assumptions about return RIP, flags and selectors. IRET handles more general saved state and is required for exceptional/compatibility cases; correctness wins over the fast path.


## 13. Lesson 02 — Linux boot and early initialization (Questions 137–146)

**137. [Beginner | Fundamentals] Why is a modern x86 bzImage more than a compressed vmlinux?**  
Answer checkpoint: It includes the x86 boot/setup protocol and decompressor code as well as the compressed kernel payload. The bootloader needs a defined handoff interface and boot parameters.

**138. [Intermediate | Fundamentals] What does the arm64 bootloader pass in x0 for the documented FDT boot path?**  
Answer checkpoint: The physical address of the DTB. x1–x3 are reserved and must be zero. The documented protocol also specifies CPU, MMU, interrupt and memory conditions.

**139. [Intermediate | Fundamentals] What is the practical difference between initrd and initramfs?**  
Answer checkpoint: The historical initrd is a block-device-style initial ramdisk image. Initramfs is a cpio archive unpacked into the initial rootfs; its /init program runs as early userspace.

**140. [Intermediate | Fundamentals] Compare rdinit=, init= and root=.**  
Answer checkpoint: rdinit= selects early userspace from initramfs; init= selects the final init executable after the normal root path is prepared; root= identifies the real root source for the kernel mount path.

**141. [Intermediate | Source/API] In v7.4-rc4, what are the major responsibilities of start_kernel() before rest_init()?**  
Answer checkpoint: Establish boot CPU state, call setup_arch(), parse the command line, initialize early memory/traps, scheduler, IRQ/timekeeping, console and core subsystems, then transition through rest_init().

**142. [Advanced | Source/API] How are initcall functions collected and ordered?**  
Answer checkpoint: Macros in include/linux/init.h place function pointers into named sections. The linker script keeps and orders those sections. init/main.c walks the resulting ranges. rootfs is a distinct subsection between fs and device ranges in this source revision.

**143. [Advanced | Source/API] Why does kernel_init_freeable() call wait_for_initramfs() before trying early userspace?**  
Answer checkpoint: populate_rootfs() may unpack the archive asynchronously. The wait ensures /init and its files are ready before the kernel checks and executes the early init path.

**144. [Intermediate | Debugging] The kernel prints “No working init found.” What do you inspect?**  
Answer checkpoint: Confirm the selected init path, executable bit, shebang interpreter, ELF ABI and dynamic loader, shared libraries, root mount, initramfs contents and rdinit=/init/init= overrides.

**145. [Advanced | Debugging] A SoC boots inconsistently after the bootloader jumps to arm64 Linux. What handoff-specific bug would you investigate?**  
Answer checkpoint: Verify all DMA-capable bus masters are quiesced, reserved-memory ranges are correct, the DTB is valid and correctly placed, cache/coherency state meets the boot protocol, and CPU/MMU/interrupt state is compliant.

**146. [Advanced | Architecture/trade-off] A storage or audio driver is required before the real root filesystem can mount. Should it be built-in or modular?**  
Answer checkpoint: Either compile the driver and its dependencies into the kernel, or include the module, firmware and dependency tooling in initramfs. Evaluate boot reliability, dependency ordering, update flexibility, image size and attack surface; a module only on the unavailable real root cannot load early enough.
