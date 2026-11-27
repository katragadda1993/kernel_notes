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

Before implementing a driver entry point, complete [Lesson 01 — The User/Kernel Boundary](lessons/00-foundations/lesson-01-user-kernel-boundary.md). A driver request path begins in a syscall or subsystem interface, while hardware completion is commonly asynchronous through IRQs, polling or deferred work. Keep request context and completion context separate in every design and trace.


## Boot-time driver availability and firmware handoff

Before probe() can run, the bootloader and kernel must agree on the hardware state:

- Boot firmware must stop or reset DMA-capable bus masters before handing RAM to Linux. This is explicitly required by the arm64 boot protocol.
- Device Tree/ACPI must describe MMIO resources, IRQs, clocks, resets, regulators, IOMMU and reserved memory consistently with the hardware.
- Drivers needed to discover or mount the real root filesystem must either be built into the kernel or be available as modules (with firmware and dependencies) inside initramfs.
- A provider that is not registered when a consumer probes may cause deferred probing; boot ordering is not a substitute for explicit framework dependencies.
- Firmware-backed IPC sessions and shared-memory channels must have a defined ownership transition from boot firmware to Linux.

For an SoC containing an audio DSP, PCIe endpoint and storage controller, write down which boot stage owns each DMA engine and reserved-memory region. The kernel must not reuse memory while firmware or a device can still access it.
