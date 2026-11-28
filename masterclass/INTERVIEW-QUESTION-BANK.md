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


## 14. Lesson 03 — C, ELF, linker and Kbuild (Questions 147–156)

**147. [Beginner | Fundamentals] Why does a driver use fixed-width kernel types such as u32 rather than assuming that unsigned long is 32 bits?**  
Answer checkpoint: unsigned long follows the target ABI and may be 32 or 64 bits. Hardware fields and wire formats have specified widths; fixed-width types make the intended width explicit. Endianness remains a separate concern.

**148. [Beginner | Fundamentals] What is the difference between a CPU physical address and a dma_addr_t?**  
Answer checkpoint: the DMA address is the device-visible address returned/managed by the DMA API and may be an IOVA translated by an IOMMU. Never derive it by casting a CPU pointer or phys_addr_t.

**149. [Intermediate | Fundamentals] Why can sizeof(struct) differ from the sum of its member sizes?**  
Answer checkpoint: the compiler inserts padding to satisfy member and overall alignment. Use sizeof() and offsetof() for the target ABI and enforce hardware layout with documented alignment/size checks.

**150. [Intermediate | Fundamentals] Why are C bitfields not a portable way to describe MMIO register bits?**  
Answer checkpoint: bitfield allocation order, packing and layout are implementation-defined. Use masks, shifts and kernel FIELD_PREP/FIELD_GET helpers, then use the proper MMIO accessors.

**151. [Intermediate | Source/API] What do __user and __iomem annotations tell the kernel developer and Sparse?**  
Answer checkpoint: they mark distinct address spaces and help catch invalid pointer use. They do not perform access or conversion; use copy_{to,from}_user() and readl()/writel() or the relevant APIs.

**152. [Intermediate | Source/API] Explain obj-y, obj-m and obj-$(CONFIG_FOO) in a Kbuild Makefile.**  
Answer checkpoint: obj-y contributes built-in objects, obj-m builds a module, and the configuration form maps y/m/n to built-in/module/omitted behavior. Composite objects use the corresponding <module>-y list.

**153. [Advanced | Source/API] What does a relocation entry in an ELF ET_REL object represent?**  
Answer checkpoint: it identifies a location, relocation type, symbol and addend that the linker or module loader must use to calculate/fix an address or value after placement is known.

**154. [Intermediate | Debugging] A module compiles but modpost reports an undefined symbol. What do you inspect?**  
Answer checkpoint: confirm the symbol is exported, the required config is enabled, the correct Module.symvers is used, symbol namespaces are imported where required, and the module is built against the intended configured output tree. A global C symbol is not automatically a module API.

**155. [Advanced | Debugging] A .ko is a valid ELF file but insmod reports Invalid module format. What are likely causes?**  
Answer checkpoint: check dmesg, vermagic, kernel release/config, architecture and ABI, symbol versioning, module signature policy and whether the module was built against the exact target kernel tree. Successful compilation is not load compatibility.

**156. [Advanced | Architecture/trade-off] When should a driver be built into vmlinux rather than shipped as a module?**  
Answer checkpoint: consider the boot dependency chain (especially root storage/filesystem), initramfs contents, firmware availability, recovery/update flexibility, image size, attack surface and deployment policy. A module needed before the real root is mounted must be built in or available from initramfs.


## 15. Lesson 04 — Source tree, Kconfig, Kbuild and reproducible builds (Questions 157–166)

**157. [Beginner | Fundamentals] What is the role of MAINTAINERS?**  
Answer checkpoint: It maps paths/subsystems to maintainers, reviewers, mailing lists, status and source trees. It is a routing guide that must be combined with subsystem documentation and current review practice.

**158. [Beginner | Fundamentals] What is the difference between Kconfig and Kbuild?**  
Answer checkpoint: Kconfig models selectable features, types and dependencies. Kbuild uses the effective configuration and directory Makefiles to compile and link selected objects.

**159. [Intermediate | Fundamentals] Why is ARCH=x86 used for an x86-64 kernel build?**  
Answer checkpoint: Kbuild's architecture identifier and source directory are x86. uname -m reports the machine ABI name and should not be copied blindly into ARCH.

**160. [Intermediate | Fundamentals] What does O= change in a kernel build?**  
Answer checkpoint: It selects a separate output tree for .config, generated headers, objects, vmlinux, modules and other artifacts, allowing clean per-architecture/per-configuration builds.

**161. [Intermediate | Source/API] What does olddefconfig do after a config fragment is merged?**  
Answer checkpoint: It resolves new/changed symbols using defaults and Kconfig dependency/choice rules without prompting. Inspect the final .config because requested fragment values may be constrained or overridden.

**162. [Intermediate | Source/API] How does merge_config.sh handle multiple fragments?**  
Answer checkpoint: It merges a base configuration and fragments in order; later assignments can override earlier values. It reports redefinitions and should be followed by olddefconfig plus a diff/inspection.

**163. [Advanced | Source/API] How would you use get_maintainer.pl when preparing a driver patch?**  
Answer checkpoint: Run it on the patch (or affected files) from the kernel source tree, inspect M/R/L/T and path matches in MAINTAINERS, then confirm the subsystem tree, documentation and recent review threads.

**164. [Intermediate | Debugging] CONFIG_MY_DRIVER=y is in a fragment, but it is absent from the final .config. What do you investigate?**  
Answer checkpoint: Confirm the symbol exists in this revision, type and depends on chain, choice constraints, architecture support, fragment order and merge warnings. Run olddefconfig and inspect the defining Kconfig entry.

**165. [Advanced | Debugging] An x86 build works, but the arm64 cross-build uses host tools or stale objects. What is the recovery plan?**  
Answer checkpoint: Use a fresh arm64 O= directory, set ARCH=arm64 and a verified CROSS_COMPILE or LLVM toolchain, inspect verbose commands and versions, and ensure no environment variable redirects Kbuild to another output tree.

**166. [Advanced | Architecture/trade-off] How would you design a reproducible kernel CI matrix for a SoC driver?**  
Answer checkpoint: Pin source revision/config fragments/toolchains; use separate output trees; combine fast compile/config checks with Sparse, KUnit, kselftest and hardware/QEMU tests. Gate on correctness-critical jobs, mark expensive coverage as scheduled/advisory, and archive configs, logs and artifact hashes.


## 16. Lesson 05 — Kernel runtime, modules, parameters and logging (Questions 167–176)

**167. [Beginner | Fundamentals] What is the difference between a loadable kernel module and a userspace shared library?**  
Answer checkpoint: A module executes in kernel privilege and uses kernel internal APIs. The module loader resolves kernel symbols, relocates sections, invokes init and manages references/unload. A userspace shared library is mapped into a process address space and follows the userspace ABI/runtime.

**168. [Beginner | Fundamentals] What happens if a module initialization function returns a negative errno?**  
Answer checkpoint: Loading fails. The init function must release every resource it acquired before returning; the module exit callback is not invoked for failed initialization.

**169. [Intermediate | Fundamentals] Why should asynchronous work be cancelled synchronously before freeing its context?**  
Answer checkpoint: The callback may still be running or may be queued to run later. cancel_delayed_work_sync() ensures pending/running execution is drained before the context or module code is released.

**170. [Intermediate | Fundamentals] What do module parameter permission bits control?**  
Answer checkpoint: They control access to the parameter's sysfs file when one is created. They do not replace validation, synchronization or the decision about which values can be supplied during module insertion.

**171. [Intermediate | Source/API] What is the purpose of module_param_cb() compared with module_param()?**  
Answer checkpoint: It supplies custom setter/getter operations, allowing validation, range checking and controlled updates rather than relying only on a standard type parser.

**172. [Intermediate | Source/API] Explain the difference between pr_debug() and dynamic debug.**  
Answer checkpoint: pr_debug() marks a debug callsite. Dynamic debug is a runtime mechanism that selectively enables eligible callsites by module/file/function/line/format when configured in the kernel.

**173. [Advanced | Source/API] Why does the lab use both a stop flag and cancel_delayed_work_sync()?**  
Answer checkpoint: The stop flag prevents a running callback from requeueing itself. Synchronous cancellation waits for pending or executing work to finish, establishing the lifetime boundary before memory is freed.

**174. [Intermediate | Debugging] A module compiles successfully but cannot be loaded on the target. What evidence should be collected first?**  
Answer checkpoint: Read the kernel log, inspect modinfo vermagic, target architecture, kernel config, symbol versions, module signature policy and the exact build tree. A successful .ko link is not proof of load compatibility.

**175. [Advanced | Debugging] A driver crashes intermittently during rmmod while a work item or IRQ is active. How do you reason about the fix?**  
Answer checkpoint: Stop new requests and hardware event production, set a stopping state, mask/synchronize IRQs, cancel work/timers synchronously, quiesce DMA and firmware ownership, then free buffers/context. Prove every asynchronous path is drained before release.

**176. [Advanced | Architecture/trade-off] Why is a module reference insufficient to prove a DMA buffer is safe to free?**  
Answer checkpoint: A module reference protects module code from unload while a user holds the reference. It does not prove the device or firmware has stopped bus mastering. Hardware must be quiesced and DMA mappings/ownership synchronized before memory is reclaimed.


## 16. Lesson 06 — Kernel memory allocation and GFP context (Questions 177–186)

**177. [Beginner | Fundamentals] Why does a GFP mask form part of the correctness contract for an allocation?**  
Answer checkpoint: It tells the allocator whether it may sleep, enter direct reclaim, perform I/O/filesystem work, use reserves or charge a memory cgroup. A mask incompatible with the caller's execution context can sleep in atomic context or cause reclaim recursion/deadlock.

**178. [Beginner | Fundamentals] Compare physical contiguity and virtual contiguity for kmalloc(), vmalloc() and kvmalloc().**  
Answer checkpoint: kmalloc returns a virtually and physically contiguous allocation; vmalloc maps potentially discontiguous pages into a contiguous kernel virtual range; kvmalloc may return either kind depending on whether its kmalloc attempt or vmalloc fallback succeeds.

**179. [Intermediate | Fundamentals] What does alloc_pages(gfp, order) request from the buddy allocator?**  
Answer checkpoint: A block of 2^order physically contiguous base pages. Higher orders are more vulnerable to external fragmentation and zone/watermark constraints.

**180. [Intermediate | Fundamentals] Why does Linux have both a buddy page allocator and SLUB?**  
Answer checkpoint: The buddy allocator manages physical page-frame blocks; SLUB efficiently allocates/frees typed objects carved from slab pages and size-class caches. They solve different granularity/performance problems.

**181. [Intermediate | Source/API] Why is GFP_KERNEL unsuitable in a timer callback or hard IRQ handler?**  
Answer checkpoint: GFP_KERNEL may enter direct reclaim and sleep. Timer and hard-IRQ contexts cannot sleep. Prefer preallocation; if a truly essential small atomic allocation is unavoidable, choose a nonblocking GFP mask and handle failure.

**182. [Intermediate | Source/API] What bug does kcalloc() or kmalloc_array() prevent compared with kmalloc(n * size, GFP_KERNEL)?**  
Answer checkpoint: They check multiplication overflow before allocation, preventing an undersized allocation followed by out-of-bounds indexing.

**183. [Advanced | Source/API] How do GFP_NOWAIT and GFP_ATOMIC differ?**  
Answer checkpoint: Both avoid sleeping/direct reclaim. GFP_ATOMIC may access emergency reserves and should be reserved for critical atomic paths; GFP_NOWAIT avoids that reserve behavior and is more likely to fail, so it needs a fallback.

**184. [Intermediate | Debugging] A warning says “sleeping function called from invalid context” after a driver adds kmalloc(GFP_KERNEL). How do you diagnose it?**  
Answer checkpoint: Trace the complete call path and execution context, including IRQ/softirq/timer and spinlock/IRQ-disabled regions. Move work to a sleepable thread/workqueue or preallocate/recycle objects; do not just replace every mask with GFP_ATOMIC.

**185. [Advanced | Debugging] A high-order page allocation fails even though /proc/meminfo shows substantial MemFree. What do you inspect?**  
Answer checkpoint: Requested order, zone/node, /proc/buddyinfo free blocks by order, /proc/pagetypeinfo, watermarks, migratetype, reclaim/compaction eligibility and fragmentation. MemFree is an aggregate and does not guarantee a suitably contiguous block.

**186. [Advanced | Architecture/trade-off] Design memory allocation for a PCIe audio engine with a descriptor ring and large streaming PCM buffers.**  
Answer checkpoint: Use the DMA API and device DMA mask. A coherent DMA allocation is a common choice for the descriptor ring; payloads use the ALSA/platform buffer path and streaming DMA mapping/SG as appropriate. Keep CPU pointers separate from DMA addresses, handle cache synchronization/ownership and quiesce DMA before freeing either allocation. Avoid vmalloc as a direct single-buffer DMA source.



## 17. Lesson 07 — Kernel object lifetime, references and error unwinding (Questions 187–196)

**187. [Beginner | Fundamentals] What does `kref_init()` establish?**  
Answer checkpoint: It initializes the embedded refcount to one. That initial reference belongs to the creator/owner and must be dropped exactly once with the matching release callback.

**188. [Beginner | Fundamentals] Why must a caller already hold a valid reference before calling `kref_get()`?**  
Answer checkpoint: Incrementing a refcount through a stale pointer is already a use-after-free. A raw lookup must be protected by a lock or a correct RCU protocol; `kref_get_unless_zero()` only helps after pointer memory is safely accessible.

**189. [Intermediate | Fundamentals] How does `refcount_t` differ from using `atomic_t` as a reference counter?**  
Answer checkpoint: refcount_t provides reference-specific overflow/underflow hardening and intentionally narrower memory-ordering semantics. It is not a general counter or a publication lock.

**190. [Intermediate | Fundamentals] Why does devres not replace kref for an object that can outlive device detach?**  
Answer checkpoint: Devres releases resources with the device's detach/failure lifecycle and does not reference-count the resource. An independently retained object needs an explicit lifetime model and must not be freed while open files/callbacks still use it.

**191. [Intermediate | Source/API] When should a driver use `ERR_PTR()`, `IS_ERR()` and `PTR_ERR()`?**  
Answer checkpoint: Pointer-returning APIs may encode a negative errno in a pointer. Test with IS_ERR before dereferencing, extract the error with PTR_ERR and preserve errors such as -EPROBE_DEFER. Treat NULL separately when the API defines it as “optional resource absent”.

**192. [Intermediate | Source/API] Does `get_device()` automatically keep private memory allocated by `devm_kzalloc()` alive after remove()?**  
Answer checkpoint: No. It protects the struct device's kobject/device release lifetime. Devres private data is still released on detach unless its lifetime is explicitly tied to a separately retained object or the device's final release design.

**193. [Advanced | Source/API] What is the difference between RCU and kref in a lockless object registry?**  
Answer checkpoint: RCU protects temporary traversal of a published pointer by pre-existing readers; kref allows a successfully acquired object to outlive the read-side critical section. Lookup plus get_unless_zero and writer removal/final reclamation must be paired correctly.

**194. [Intermediate | Debugging] A driver gets a KASAN use-after-free during remove() while a firmware callback and delayed work may still run. What is your investigation order?**  
Answer checkpoint: Stop new submissions/events, mark the object stopping, disable hardware sources, quiesce DMA/firmware, unregister callbacks, synchronize IRQs, cancel/flush work and timers, then drop each owner reference and free. Build an ownership ledger and prove every asynchronous producer is drained.

**195. [Advanced | Debugging] `cancel_delayed_work_sync()` returns false during teardown. Should remove() drop the callback's reference itself?**  
Answer checkpoint: Not if the callback was running or had already completed: the synchronous API waits for a running callback, and that callback owns its put. Only when cancellation returns true for a pending item that will not execute should the cancellation path drop the queued-work reference. Never double-put.

**196. [Advanced | Architecture/trade-off] Design lifetime for an audio DSP device whose PCM stream can remain open while the physical device is unbound.**  
Answer checkpoint: Separate the driver binding's devres resources from the stream/private state retained by open users. Stop new operations, mark the device dead, stop DMA and firmware access, unregister/disable callbacks, drain IRQ/work and detach the hardware. Keep the stream state alive with explicit references until each open user releases it; do not retain devm pointers or DMA buffers past detach.
