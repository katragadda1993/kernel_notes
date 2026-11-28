# Lesson 05 — Kernel Runtime Essentials: Modules, Parameters, Logging and Safe Teardown

**Stage:** 2 — Kernel C and essential APIs  
**Time:** 75–90 minutes  
**Source baseline:** Linux v7.4-rc4  
**Prerequisites:** Lessons 01–04  
**Lab target used for compilation:** Debian kernel headers 6.12.107+deb13-amd64  
**Important:** the lab module was compiled and inspected, but not loaded. The authoring host runs 6.18.57, so its running-kernel ABI does not match the 6.12.107 build headers.

## 1. Learning objectives

After this lesson, you should be able to:

- Explain the loadable-module lifecycle from userspace request through relocation, initialization, live state and removal.
- Implement module_init() and module_exit() with correct failure unwinding.
- Define module parameters with appropriate permissions, descriptions and validation.
- Choose pr_*() versus dev_*() logging and control pr_debug() with dynamic debug.
- Explain why asynchronous work must be cancelled synchronously before module-owned memory or code can disappear.
- Diagnose common module load errors without treating a successful .ko build as proof of load compatibility.
- Apply the same lifetime and logging rules to platform, PCIe, SoundWire, ALSA and DSP drivers.

## 2. Why modules exist

A kernel can contain code built directly into the kernel image or code loaded later as a module.

Modules let a deployment add optional hardware support without rebuilding the entire kernel. They are useful for removable devices, optional subsystems, vendor/platform feature sets and development iteration. They also create extra runtime obligations: the kernel must resolve symbols, apply relocations, run initialization, track references and ensure no callback can execute after the module's text/data is freed.

A module is not a userspace shared library. It executes in kernel privilege, uses kernel APIs and can corrupt the whole system if it violates kernel invariants.

### Built-in versus loadable

| Property | Built-in object | Loadable module |
|---|---|---|
| Link location | Linked into vmlinux | Separate .ko ELF object |
| Initialization | Initcall sequence | Module loader invokes module init |
| Exit callback | No runtime unload; module_exit() is discarded for built-in code | Called on successful unload |
| Dependencies | Resolved at kernel link/configuration time | Symbols and module dependencies resolved at load time |
| Update/removal | Requires kernel image/boot lifecycle | Can load/unload when references and policy permit |
| Early boot | Available before root filesystem if built in | Must be available in initramfs if needed before real root |

A storage or filesystem driver needed to mount the real root must be built in, or its module and dependencies must be available in the initramfs. A module located only on the unavailable real root cannot bootstrap that root.

## 3. Module lifecycle and call path

A typical userspace insmod/modprobe flow is:

1. Userspace opens the .ko and requests module loading through finit_module() (or the older init_module() interface).
2. The kernel validates the ELF module image and its metadata.
3. The module loader allocates module memory, resolves required exported symbols, applies relocations and prepares module sections.
4. The loader calls the module's initialization function.
5. If initialization succeeds, the module becomes live and can be used by other kernel code.
6. On unload, the kernel checks references/availability, runs the exit function and releases module memory.

Pinned implementation entry points:

- [finit_module() and module loading — kernel/module/main.c](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/module/main.c)
- [Module declarations and module_init()/module_exit() — include/linux/module.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/module.h)
- [Module parameter declarations — include/linux/moduleparam.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/moduleparam.h)
- [Parameter parsing and sysfs exposure — kernel/params.c](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/params.c)

### Initialization and failure rules

- module_init(fn) declares the module's initialization entry point. For built-in code, the same macro participates in the initcall mechanism.
- module_exit(fn) declares the unload callback for a loadable module. Built-in code has no ordinary module-unload phase.
- A successful init function returns 0.
- A failed init function returns a negative errno such as -ENOMEM, -EINVAL or -EIO.
- **If module initialization fails, the module's exit callback is not called.** The init function must release every resource it acquired before returning an error.
- Cleanup should normally run in reverse acquisition order.

For a built-in driver, __init code can be reclaimed after initialization. For a loadable module, init-only sections can be discarded after successful module initialization; exit code must remain available for unload.

## 4. Module parameters

Parameters are useful for controlled load-time configuration and small diagnostic knobs. They are not a replacement for a subsystem's stable userspace ABI.

Common form (illustrative):

    static bool trace;
    module_param(trace, bool, 0644);
    MODULE_PARM_DESC(trace, "Enable extra diagnostic output");

A custom callback is appropriate when a value needs validation:

    static int repeat_set(const char *val, const struct kernel_param *kp)
    {
            unsigned int parsed;
            int ret = kstrtouint(val, 0, &parsed);

            if (ret)
                    return ret;
            if (parsed < 1 || parsed > 20)
                    return -ERANGE;

            WRITE_ONCE(*(unsigned int *)kp->arg, parsed);
            return 0;
    }

A matching getter can use READ_ONCE() for the same scalar. The lab implements both callbacks and registers them with module_param_cb().

### Parameter permissions

The permission argument is an octal sysfs mode:

- 0444: readable by all, not writable through the module parameter sysfs file.
- 0644: owner-readable/writable, group/other-readable.
- 0400: owner-readable only.
- 0: do not create the parameter's sysfs file.

The load operation can still supply a read-only parameter at insertion time. Permissions describe the sysfs interface; they do not make an unsafe parameter safe.

The lab parameters are:

| Parameter | Mode | Purpose |
|---|---:|---|
| message | 0444 | Read-only string copied into module-owned memory |
| repeat | 0644 | Validated scalar, updated with WRITE_ONCE() and read with READ_ONCE() |
| fail_after_alloc | 0400 | Load-time error injection after allocations |



### Built-in module-parameter timing trap

[Commit ed75528c095bc7760a3cf34a3f3e2ec7a59eb285](https://github.com/torvalds/linux/commit/ed75528c095bc7760a3cf34a3f3e2ec7a59eb285) fixed an early-boot hang when the NVMe PCI driver was built in and the nvme.quirks kernel command-line parameter was supplied.

The commit message explains that module_param_cb() caused the built-in parameter callback to run at initcall level -1. The callback parsed/allocated a string before the memory allocators were initialized. The fix changed the declaration to device_param_cb(), which schedules parsing later at device initcall level 6.

**Engineering lesson:** do not assume a module-parameter setter always runs after normal device and memory infrastructure is ready. Built-in and loadable forms can have different initialization timing. Keep early parameter callbacks allocation-free unless the selected parameter mechanism guarantees a later phase; inspect the macro expansion and initcall level in the exact source revision.

A scalar READ_ONCE()/WRITE_ONCE() pair is not a general lock. If a parameter changes a multi-field invariant, use a suitable lock or another explicit synchronization scheme. Keep parameter callbacks short, validate input and return useful errno values.

## 5. Logging: printk, pr_* and dev_*

printk() writes to the kernel log buffer; it is not the C library's printf(). Prefer the level-specific wrappers:

| Helper | Typical use |
|---|---|
| pr_emerg() / pr_alert() / pr_crit() | Severe system-level conditions |
| pr_err() | Operation failed |
| pr_warn() | Unexpected but recoverable condition |
| pr_notice() | Important normal event |
| pr_info() | Lifecycle or useful informational event |
| pr_debug() | Debug-only callsite |
| dev_err() / dev_warn() / dev_info() / dev_dbg() | Device-associated messages |

Use #define pr_fmt(fmt) KBUILD_MODNAME ": " fmt to give a module's messages a consistent prefix.

Once a driver has a struct device, prefer dev_*() because the log includes device identity and can be filtered by device. Before a device object exists, module-level pr_*() is appropriate.

Avoid printing from high-frequency IRQ, DMA completion or audio period paths unless rate-limited and necessary. Excessive logging changes timing, floods the ring buffer and can hide the original failure. Prefer counters, tracepoints or dynamic debug for detailed diagnosis.

Reference: [printk basics, v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/printk-basics.rst).

### Logging context must outlive the message

[Commit 018e3bf95e6360b7d528d3094b5302fb4e2fbed0](https://github.com/torvalds/linux/commit/018e3bf95e6360b7d528d3094b5302fb4e2fbed0) fixed a NULL dereference in USB gadget Ethernet debug logging. During unbind, the gadget pointer was cleared while the net_device survived. The old logging macros dereferenced the gadget's device; the fix used netdev_*() helpers whose net_device context remained valid for that object's lifetime.

**Engineering lesson:** logging is executable code. A debug macro can dereference pointers and trigger a crash even when the log level is disabled in ordinary use but enabled by dynamic debug. Choose a logging context whose lifetime covers the callsite; never assume a diagnostic macro is harmless after unbind.



## 6. Dynamic debug

pr_debug() and dev_dbg() callsites can be enabled selectively when the kernel is configured with dynamic debug support.

Typical workflow:

    sudo mount -t debugfs none /sys/kernel/debug
    grep lesson05_runtime /sys/kernel/debug/dynamic_debug/control
    echo 'module lesson05_runtime +p' | sudo tee /sys/kernel/debug/dynamic_debug/control

To disable those callsites:

    echo 'module lesson05_runtime -p' | sudo tee /sys/kernel/debug/dynamic_debug/control

The control syntax supports selectors such as module, file, function, line and format. Availability depends on CONFIG_DYNAMIC_DEBUG or the relevant dynamic-debug core configuration in the target kernel.

Dynamic debug is distinct from the global printk console log level:

- Console log level controls which messages are printed to the console.
- Dynamic debug controls whether selected debug callsites are enabled.
- dmesg reads the kernel log buffer; system logging services may also collect it.

Reference: [Dynamic debug HOWTO, v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/admin-guide/dynamic-debug-howto.rst).

## 7. Asynchronous work and module lifetime

The lab schedules a delayed work item. The callback executes later on a workqueue, not inside module_init().

This creates a lifetime dependency:

- The callback's instruction bytes belong to the module.
- Its container_of() context points to module-owned memory.
- Its copied message must remain valid until the callback has finished.
- The callback may reschedule itself.

The exit path therefore performs this order:

1. Set a stop flag so a running callback will not requeue itself.
2. Call cancel_delayed_work_sync() to cancel pending work and wait for a running callback to finish.
3. Free the copied message and context.
4. Return from module exit; only then can module code/data be released.

Never replace cancel_delayed_work_sync() with an asynchronous cancellation when the callback can still access memory being freed. Never free the work context before synchronizing with the callback.

The workqueue API is documented in [workqueue.rst, v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/workqueue.rst).

### Synchronous cancellation is context-sensitive

[Commit b3a3f241dace3d4a775e1401b53a3f6d304534f0](https://github.com/torvalds/linux/commit/b3a3f241dace3d4a775e1401b53a3f6d304534f0) documents a framebuffer-console path where cancel_delayed_work_sync() could be reached from atomic console-printing context and sleep illegally. The fix separated a non-synchronous cancellation used in atomic-safe paths from a synchronous cancellation used when release was about to free the callback's context. It also added state/locking so a running callback could not re-arm itself after stop.

**Engineering lesson:** cancel_delayed_work_sync() is a teardown/lifetime primitive that may sleep; it must not be called from hard IRQ, other atomic context or a path that cannot block. If the current context cannot wait, stop/requeue-gate the producer there and arrange a later process-context synchronization before freeing memory.



### Module references

Kernel subsystems can hold a reference to a module while its operations are in use. For example, a file-operations table commonly sets .owner = THIS_MODULE so an open file prevents the module text from being unloaded while its methods may still be called.

Do not add ad-hoc try_module_get()/module_put() pairs without understanding the owning subsystem's reference contract. Reference leaks prevent unload; missing references can permit use-after-free.

## 8. Error handling and resource unwinding

The lab acquires:

1. A context allocation.
2. A copied message string.
3. A delayed-work object embedded in the context.

If the injected failure is selected after allocation, it frees the message and then the context before returning -EIO. The work was initialized but never queued, so no asynchronous cancellation is needed on that failure path.

For real drivers, extend the same reasoning to:

- MMIO mappings and register windows.
- IRQ registrations.
- DMA mappings and coherent rings.
- Clocks, resets, regulators and runtime-PM references.
- Firmware handles and IPC endpoints.
- Workqueues, timers, tasklets and threaded callbacks.

Use devres/managed APIs where a struct device lifecycle makes them appropriate, but do not assume devres automatically stops asynchronous hardware activity. The device must be quiesced before memory and resources are released.

## 9. Lab architecture and commands

The lab source is [lesson05_runtime.c](lesson-05-kernel-runtime-modules-logging-debugging/lesson05_runtime.c). Its Makefile accepts an explicit target build tree.

### A. Build against the exact target kernel

    make KDIR=/path/to/exact/kernel/build clean all
    modinfo ./lesson05_runtime.ko
    readelf -S ./lesson05_runtime.ko | grep -E '\\.init.text|\\.exit.text|\\.modinfo|__param'

Expected evidence:

- Kbuild compiles and links lesson05_runtime.ko.
- modinfo lists message, repeat and fail_after_alloc.
- The ELF object contains module metadata and parameter sections. The exact section layout is toolchain/configuration dependent.

### B. Load only in a disposable matching VM or board

**Never load a module into a production system. Never load it if its target kernel build does not match the running kernel.**

    sudo insmod ./lesson05_runtime.ko message="DSP diagnostic heartbeat" repeat=5
    cat /sys/module/lesson05_runtime/parameters/repeat
    echo 2 | sudo tee /sys/module/lesson05_runtime/parameters/repeat
    sudo dmesg | tail -n 30
    sudo rmmod lesson05_runtime

The worker logs through pr_debug(). Enable dynamic debug as described above to see its work-item messages. The work item is finite; increasing repeat after it has already completed does not restart it. Reload the module for a fresh run.

### C. Exercise the init failure path

    sudo insmod ./lesson05_runtime.ko fail_after_alloc=1

Expected behavior on a matching test kernel: insertion fails with EIO; the log reports the injected failure; no module remains loaded. The init function must have freed its allocations before returning.

### D. Inspect the module metadata

    modinfo ./lesson05_runtime.ko
    readelf -h ./lesson05_runtime.ko
    readelf -S ./lesson05_runtime.ko

Compare .init.text, .exit.text, .modinfo and __param. Do not infer runtime correctness solely from section presence.

## 10. Debugging scenarios

### Scenario 1 — insmod says “Invalid module format”

Inspect dmesg, modinfo, vermagic, architecture, kernel configuration, symbol versions and signature policy. Confirm the module was built against the exact target kernel output tree, not merely headers from a different installed release.

### Scenario 2 — rmmod hangs

Look for a callback that never finishes, work that keeps requeueing, a timer that is not shut down, a thread that is not stopped or a reference held by an open file/device operation. Add a stop state, stop the producer and synchronously drain asynchronous execution before freeing context.

### Scenario 3 — Module loads but pr_debug() is invisible

Check whether dynamic debug is enabled in the target kernel, whether debugfs is mounted, whether the callsite is registered and whether the query matches the module/file/function. Do not confuse dynamic-debug enablement with console log-level filtering.

### Scenario 4 — A DSP driver crashes only during remove

Trace the exact order: stop new userspace/subsystem requests, stop DSP/firmware DMA, mask and synchronize interrupts, cancel work/timers, close IPC endpoints, release PM references, then free rings and private state. A module reference protects module code, not automatically the lifetime of every device buffer or firmware-owned object.

## 11. SoC and audio-driver application

Consider an SoC with an audio controller, SoundWire codec and DSP firmware:

- Use dev_info() for probe and selected configuration details after the device exists.
- Use dev_dbg()/dynamic debug for register programming and stream-state diagnostics.
- Avoid printing every PCM period or SoundWire frame; use tracepoints/counters for high-rate paths.
- Do not free DMA descriptors merely because rmmod has started. Stop the engine, verify hardware is idle, synchronize IRQs and work, then unmap/free buffers.
- If the DSP can still access shared memory, firmware/IPC shutdown and ownership transfer must complete before host memory is reclaimed.
- Runtime-PM references and clocks must remain valid while register access or firmware shutdown requires them.

The same reasoning applies to PCIe, USB, I2C/SPI and network drivers: teardown is a protocol between Linux, asynchronous software execution and the hardware.

## 12. Exercises

- **Beginner:** add a read-only u32 parameter and describe how its value is exposed through sysfs.
- **Intermediate:** add a validated parameter callback with a range check and a test for invalid input.
- **Intermediate:** deliberately add a workqueue requeue race, then repair it using a stop flag and synchronous cancellation.
- **Advanced:** write an error-unwind table for a hypothetical PCIe driver that allocates a DMA ring, requests MSI-X, starts a work item and enables bus mastering.
- **Advanced:** explain why a module reference alone does not prove that hardware has stopped DMA to a buffer.
- **Senior/staff:** design a remove path for a DSP driver where firmware may be unresponsive. Define timeout, reset, DMA isolation and memory-retention behavior; do not free memory while an uncontrolled bus master can still access it.

## 13. Knowledge check

1. Why is a .ko file not equivalent to a userspace shared library?
2. What happens if a module's initialization function returns a negative errno?
3. Why is module_exit() not a substitute for init-failure unwinding?
4. What is the difference between a module parameter's sysfs permission and whether it can be supplied at load time?
5. When should a driver use dev_err() rather than pr_err()?
6. What does dynamic debug control that printk console log levels do not?
7. Why must cancel_delayed_work_sync() complete before freeing a work item's context?
8. Why can a module reference prevent code unload but still fail to protect a DMA buffer?
9. What should be checked before loading a module built against a different kernel header package?
10. Why is a rate-limited log or tracepoint often preferable to pr_info() in an audio period callback?

## 14. References pinned to v7.4-rc4

- [Module loader implementation](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/module/main.c)
- [Module API and entry macros](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/module.h)
- [Module parameter API](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/moduleparam.h)
- [Parameter parsing/sysfs implementation](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/params.c)
- [Printk basics](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/printk-basics.rst)
- [Dynamic debug HOWTO](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/admin-guide/dynamic-debug-howto.rst)
- [Workqueue documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/workqueue.rst)
- [Kernel parameter documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/admin-guide/kernel-parameters.rst)

## 15. Validation record

The module was built using:

- Compiler/build environment: Debian kernel headers 6.12.107+deb13-amd64
- Host running kernel: 6.18.57
- Architecture: x86-64
- Command: make KDIR=/usr/src/linux-headers-6.12.107+deb13-amd64 clean all

Observed results:

- Kbuild compiled, modposted and linked lesson05_runtime.ko.
- modinfo showed the expected name, version, vermagic and three parameters.
- readelf showed .init.text, .exit.text, .modinfo and __param sections.

Not performed:

- Module insertion/removal: intentionally not attempted because the module vermagic targets 6.12.107 while the host runs 6.18.57.
- Runtime workqueue/dynamic-debug behavior and injected init failure.
- Sparse (not installed), full v7.4-rc4 kernel build, arm64 cross-build, QEMU and hardware validation.

A successful external-module build proves compilation/linking against the selected headers. It does not prove load compatibility, runtime correctness, DMA safety or hardware behavior.
