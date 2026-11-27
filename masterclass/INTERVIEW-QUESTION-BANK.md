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