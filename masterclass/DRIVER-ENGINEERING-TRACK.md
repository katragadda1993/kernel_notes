# Driver Engineering Track: Specification to Upstream

This track follows a hardware driver's full lifecycle, not just its probe function.

## Lifecycle
1. **Understand the hardware contract** — programming model, register specification, reset values, clocks, power, interrupts, DMA, coherency, firmware and errata.
2. **Map the Linux subsystem** — identify bus, framework, existing drivers, bindings, maintainers and expected userspace ABI.
3. **Describe the device** — Device Tree/ACPI, resources, interrupts, clocks, resets, regulators, IOMMU and power domains.
4. **Probe safely** — validate resources, map MMIO, configure DMA masks, initialize locks/state and handle deferred probe.
5. **Initialize hardware** — reset sequencing, clock/power sequencing, register programming and readiness polling.
6. **Move data** — PIO or DMA, descriptor formats, ownership, barriers, cache synchronization and completion.
7. **Handle events** — IRQ top/bottom halves, threaded handlers, workqueues, timeouts and error recovery.
8. **Expose the right interface** — use the relevant kernel subsystem; avoid inventing a private ioctl ABI where a standard framework exists.
9. **Manage power** — runtime PM, system sleep, wakeup, clock gating and state restoration.
10. **Remove and recover** — stop producers, quiesce DMA, synchronize IRQ/work, release references and handle hot-unplug/reset.
11. **Validate** — normal/error paths, concurrency, fault injection, sanitizers, stress, suspend/resume and performance.
12. **Upstream** — bindings, documentation, maintainers, minimal patch series, review and stable backport considerations.

## Required design checklist
- Register access ordering, endianness and reserved-bit handling
- DMA mask, mapping direction, coherency and descriptor ownership
- IRQ acknowledgement, shared interrupt behavior and teardown synchronization
- Object lifetime, reference counting and asynchronous callback cancellation
- Probe error unwinding and deferred-probe behavior
- Reset and recovery state machine
- Runtime/system PM ordering and failure handling
- Userspace ABI stability and subsystem fit
- Tests that demonstrate both success and failure behavior

## Capstone sequence
1. Character driver and lifetime rules
2. Platform driver with Device Tree, MMIO and IRQ
3. DMAengine client with cyclic and scatter-gather transfers
4. PCIe endpoint driver with MSI/MSI-X and recovery analysis
5. I2C/SPI client with retries and fault injection
6. ALSA PCM/ASoC path with DMA and XRUN diagnosis
7. remoteproc/RPMsg/mailbox IPC integration
8. Runtime PM and remove-path race investigation
9. DMA corruption and cache-coherency root-cause exercise
10. Upstream patch from issue reproduction through review-ready submission

Each capstone must run in QEMU, a mock/test framework, or documented real hardware. Clearly mark hardware-dependent steps; never fabricate results.


## Foundation prerequisite

Before implementing a driver entry point, complete [Lesson 01 — The User/Kernel Boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md) and [Lesson 02 — Linux Boot and Early Initialization](lessons/00-foundations/lesson-02-linux-boot-and-early-init.md). A driver request path begins in a syscall or subsystem interface, while hardware completion is commonly asynchronous through IRQs, polling or deferred work. Lesson 02 adds the earlier bootloader-to-kernel hardware ownership contract, including DMA quiescence, firmware descriptions and initramfs driver availability. Keep request context and completion context separate in every design and trace.


## Boot-time driver availability and firmware handoff

Before probe() can run, the bootloader and kernel must agree on the hardware state:

- Boot firmware must stop or reset DMA-capable bus masters before handing RAM to Linux. This is explicitly required by the arm64 boot protocol.
- Device Tree/ACPI must describe MMIO resources, IRQs, clocks, resets, regulators, IOMMU and reserved memory consistently with the hardware.
- Drivers needed to discover or mount the real root filesystem must either be built into the kernel or be available as modules (with firmware and dependencies) inside initramfs.
- A provider that is not registered when a consumer probes may cause deferred probing; boot ordering is not a substitute for explicit framework dependencies.
- Firmware-backed IPC sessions and shared-memory channels must have a defined ownership transition from boot firmware to Linux.

For an SoC containing an audio DSP, PCIe endpoint and storage controller, write down which boot stage owns each DMA engine and reserved-memory region. The kernel must not reuse memory while firmware or a device can still access it.


## Build and ABI prerequisite

Complete [Lesson 03 — C, ELF, Linker and Kbuild Foundations](lessons/00-foundations/lesson-03-c-elf-kernel-build.md) before implementing a hardware-facing driver. Record the target architecture, compiler, kernel commit, .config, generated headers and Module.symvers source.

For every hardware-visible structure, document size, member offsets, alignment, endianness and ownership. Distinguish CPU virtual, CPU physical and device DMA addresses. Use __iomem accessors for registers, endian conversions for device formats and the DMA API for mappings. Do not treat __packed or a successful x86 build as proof of portable hardware layout.

For a driver built into vmlinux, verify its Kconfig dependency and initcall/provider ordering. For a module, verify vermagic, exported symbols, namespaces, symbol versions and firmware availability. Never load a module built against a different target kernel merely because it compiled.


## Source-tree and build-environment prerequisite

Complete [Lesson 04 — Linux Kernel Source Tree, Configuration and Reproducible Builds](lessons/00-foundations/lesson-04-kernel-tree-config-reproducible-builds.md) before beginning an in-tree driver change. Identify the owning subsystem using MAINTAINERS and get_maintainer.pl; inspect the subsystem tree and recent accepted patches. Keep one output directory per architecture/configuration, pin the source revision and config fragments, and record the exact compiler/binutils versions.

For SoC work, the same source revision can be built for a host-side mock/test configuration and an arm64 target, but these are distinct artifacts. Do not reuse x86 generated headers or objects in an arm64 output tree. A successful build only proves the selected compile/link paths; it does not prove probe ordering, firmware availability, register behavior, DMA coherency or hardware functionality.


## Module runtime and safe-debugging prerequisite

Complete [Lesson 05 — Kernel Runtime Essentials](lessons/00-foundations/lesson-05-kernel-runtime-modules-logging-debugging.md) before writing an asynchronous driver. Understand init failure unwinding, parameter validation, device-aware logging, dynamic debug and module references. Every work item, timer, IRQ thread and firmware callback needs an explicit stop/synchronize/free sequence. A module reference protects code lifetime; it does not prove that DMA or firmware access to a buffer has stopped.


## Memory allocation and DMA contract prerequisite

Complete Lesson 06 — Kernel Memory Allocation and GFP Context before choosing driver buffer allocation APIs.

For each allocation, record object size, alignment and overflow safety; CPU virtual and physical contiguity requirements; device DMA requirements and the DMA API used; execution context and reclaim/sleep permission; possible reclaim recursion into filesystem or I/O; ownership, failure unwind and the point all asynchronous users stop.

Typical SoC/PCIe/audio split:
- CPU-only control structures: kzalloc() or a typed kmem_cache;
- hardware descriptor ring: commonly dma_alloc_coherent() and its returned DMA handle;
- streaming payload: subsystem buffer API plus streaming DMA mapping/SG as appropriate;
- large CPU-only tables: kvzalloc() only when physical contiguity is unnecessary.

Never cast a CPU pointer to dma_addr_t, assume vmalloc() memory is one physical segment, or free devm memory while a DMA engine/IRQ/work item/firmware callback can still access it.


## Object lifetime and reference ownership

Complete [Lesson 07 — Kernel Object Lifetime, References and Error Unwinding](lessons/00-foundations/lesson-07-object-lifetime-references-error-unwinding.md) before exposing driver-private state to multiple asynchronous users.

For each object, maintain an ownership ledger:

- creator/driver binding reference;
- registry or subsystem publication reference;
- one reference per open file/stream that can outlive the initiating call;
- one reference per queued callback or asynchronous request;
- references retained by firmware/remote processors only when the protocol explicitly models such ownership.

Use `kref`/`refcount_t` for memory lifetime, locks for mutable state, RCU for safe lockless publication/lookup where justified, and subsystem/device references for their corresponding objects. These mechanisms are not interchangeable.

Devres resources are tied to device detach and are not reference-counted. Do not let an open PCM stream, sysfs callback or firmware completion retain a pointer to `devm_kzalloc()` memory after unbind. `get_device()` keeps the device object alive but does not automatically extend the lifetime of arbitrary devres private data.

### Teardown order to prove

1. Stop accepting new users/requests and mark the state as stopping.
2. Unpublish subsystem/registry interfaces so no new lookup/open can start.
3. Stop the hardware/firmware producer; quiesce bus mastering and DMA.
4. Mask and synchronize IRQs and unregister callbacks/notifiers.
5. Cancel/flush work and timers in a sleepable context; drain chained work before destroying its queue.
6. Drop registry, device and asynchronous references exactly once.
7. Free DMA/private memory only after the last software and hardware user is gone.
8. Let devres release the remaining device-bound resources after `remove()` returns.

For PCIe audio/DSP designs, keep the descriptor ring, streaming payload, stream state and firmware IPC context as separate ownership objects. A stream reference must not accidentally keep a removed MMIO mapping or active DMA buffer alive.
