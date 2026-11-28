# Linux Kernel Source Archaeology

Learn not only what the kernel does, but why the design evolved.

## Evidence hierarchy
1. Merged mainline commit and its full diff
2. Subsystem maintainer pull request and merge message
3. Mailing-list review discussion on lore.kernel.org
4. Kernel documentation and maintainer documentation
5. LWN or conference talks as explanatory context
6. Engineering interpretation, clearly labelled as interpretation

## Repeatable investigation
1. Identify the exact symbol, file, subsystem and kernel revision.
2. Inspect current implementation and relevant callers/callees.
3. Use `git log --follow`, `git blame`, and commit search to find introduction and later changes.
4. Read the full commit message, parent revision and diff.
5. Follow the subsystem pull request and review discussion where available.
6. Identify the original problem, constraints, rejected alternatives and compatibility requirements.
7. Compare before/after call paths, data structures, locking, lifetime and error handling.
8. Build a minimal reproducer or test when practical.
9. Record whether the claim is documented fact, maintainer statement, observed code behavior or interpretation.

## Required lesson note
- Kernel tag/commit:
- Subsystem and maintainers:
- Original problem:
- Previous implementation:
- New implementation:
- Why the change was made (with evidence):
- Alternatives/trade-offs:
- Driver or hardware impact:
- Regression risks and tests:
- What remains uncertain:

## Useful upstream resources
- [Mainline Git repository](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/)
- [Linux kernel documentation](https://docs.kernel.org/)
- [Kernel mailing-list archive](https://lore.kernel.org/)
- [Kernel development process](https://docs.kernel.org/process/2.Process.html)
- [Kernel releases](https://www.kernel.org/category/releases.html)

Never infer intent solely from a commit title. Do not attribute proprietary hardware design rationale to upstream maintainers without direct evidence.


## Current case study: DMA-mask workaround versus platform capability

**Question:** How can a hardware workaround make a driver fail to probe on a platform where the device previously worked?

- Earlier change: [614f4d166eee — set a 31-bit coherent mask for BCM57766](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/614f4d166eee).
- Follow-up fix: [686b8dbcedf8773dc505f9f315a0551c0bca70a3](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/686b8dbcedf8773dc505f9f315a0551c0bca70a3).
- Original maintainer patch explanation: [netdev discussion](https://lists.openwall.net/netdev/2026/11/08/62).

The patch explains that direct DMA support can reject a 31-bit coherent mask when physical memory extends above 2 GiB and CONFIG_ZONE_DMA is absent. The revised driver treats the mask as best-effort and falls back when it is unsupported, while logging the condition.

**Evidence-based lesson:** a device workaround must be evaluated against the DMA API and platform topology. A driver should distinguish “the workaround cannot be applied” from “the device cannot function at all,” and should report any remaining erratum exposure.

## Current case study: AF_XDP buffer ownership in a failure path

[dpaa2-eth commit 25faec9ce61c01fb45abf812a222c5b43c3cd5c5](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/25faec9ce61c01fb45abf812a222c5b43c3cd5c5) documents how simplifying cleanup after an objtool warning exposed a real ownership bug. A loop index was also the count of successfully mapped buffers; using it for cleanup changed the ownership count and could cause buffers to be released to hardware after being freed.

Use this as a review exercise:
- Identify every state transition for each buffer.
- Separate allocation count, mapping count, completion count and cleanup iterator.
- Prove that only successfully mapped buffers are handed to hardware.
- Exercise partial allocation and partial mapping failures.


## Current case study: bootloader handoff and early userspace

### x86 boot protocol evolution — documented history

The [v7.4-rc4 x86 boot protocol document](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/x86/boot.rst) explicitly attributes the protocol's complexity to the historical PC memory model, the bootable-image requirement and changing firmware/PC expectations. It records protocol 2.00 as adding bzImage and initrd support with a formal bootloader handoff, and later revisions as adding relocatability, command-line capacity and high-memory support.

### initramfs versus initrd — code-history evidence

The [initramfs buffer-format document](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/driver-api/early-userspace/buffer-format.rst) says the initramfs protocol complemented the older initial ramdisk protocol in the 2.5 development series. A [2014 LZ4 decompression fix](https://github.com/torvalds/linux/commit/4d4b866aee039d609c0b40e7e5b27204607ce614) explains the different input paths used for preboot, initramfs and initrd. This is evidence that the two formats have distinct data-handling semantics, not merely different names.

### arm64 boot-time page-table ordering — merged mainline evidence

[Commit 822ba3a89c7896b26e4beb70e2ebffb881924dfe](https://github.com/torvalds/linux/commit/822ba3a89c7896b26e4beb70e2ebffb881924dfe) adds an ISB after mapping the FDT in the initial identity map and fixes break-before-make/barrier behavior for fixmap and KPTI-related mappings. The maintainer pull message identifies these as missing ordering requirements. The engineering lesson is that writing a page-table entry, making a translation visible to other observers, invalidating stale TLB state and synchronizing instruction execution are separate obligations.

### Engineering interpretation

Early userspace reduces the need to place every storage, network, filesystem and device-discovery workflow inside the kernel. It does not remove the need for those drivers: the kernel must still have enough built-in support to expose the devices needed to load the modules and mount the real root. The right built-in-versus-module decision depends on the dependency chain and failure-recovery requirements.


## Current case study: initcall order is part of the build contract

[Commit 20dff14006b189baf05011b44e785d100458cc95](https://github.com/torvalds/linux/commit/20dff14006b189baf05011b44e785d100458cc95), merged 2026-10-06, explains a real dependency between network initialization functions. A previous arrangement put inet6_init() at device_initcall level. The commit explains that blackhole_netdev_init() had to run first, and that built-in driver initialization at the same level could otherwise precede a consumer that expected blackhole_netdev to exist. The fix explicitly orders blackhole_netdev_init() at subsys_initcall_sync(), inet_init() at fs_initcall(), and inet6_init() at fs_initcall_sync().

The source commit also explains why an earlier stack trace had led to an inaccurate description of the race. This is a useful source-archaeology lesson: read the commit message and trace the actual dependency rather than repeating a shorthand explanation.

**Engineering lesson:** Kbuild object order and linker-collected initcall sections can influence when built-in initialization runs. Prefer explicit framework dependencies or appropriate initcall levels over relying on incidental ordering.

## Current case study: GNU Make jobserver and Clang LTO

[Commit ae5f6044c4cdffe2b3361b15c1d6fc67b339f2a4](https://github.com/torvalds/linux/commit/ae5f6044c4cdffe2b3361b15c1d6fc67b339f2a4), merged 2026-10-11, fixes a Kbuild recipe that generated initcall ordering for Clang LTO. After an earlier refactor moved the command into a different Makefile recipe, it no longer had the recursive-recipe '+' prefix. GNU Make therefore did not pass its jobserver pipe to the child process. The helper could not reopen the jobserver and fell back to a CPU-count-based parallelism limit rather than the available free job slots.

The fix restores the '+' prefix. The commit includes a concrete reproduction context (x86_64 defconfig, CONFIG_LTO_CLANG_THIN, LLVM=1 and GNU Make 4.3 with a close-on-exec backport), a Fixes reference, review and testing tags, and an Assisted-by: LLM attribution.

**Documented fact:** the commit message describes the missing jobserver descriptor and the exact Makefile change.  
**Engineering interpretation:** build-system correctness includes process/FD inheritance and jobserver semantics, not only compiler flags. An AI-assisted patch is still subject to a reproducible failure, review and testing evidence.

Related review discussion: [Kees Cook's initcall-ordering discussion](https://lore.kernel.org/all/CAK7LNARCM=rUm8mA8GRQ7ufeyfneGf4OEvHmESKt=zuxs2KrHw@mail.gmail.com/).


## Lesson 04 — Source ownership and build configuration archaeology

The source-tree/build-system investigation should be evidence-based:

- Start with the pinned [Kbuild overview](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kbuild.rst), [Kconfig language](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kconfig-language.rst), [MAINTAINERS](https://github.com/torvalds/linux/blob/v7.4-rc4/MAINTAINERS) and [reproducible-build documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/reproducible-builds.rst).
- Use git log --follow and git blame on the actual Kconfig/Makefile/source file to understand why a symbol or object is selected.
- Inspect the source revision's get_maintainer.pl output and current subsystem review instructions; ownership can change over time.
- Record the exact source commit and final .config. A defconfig name alone is insufficient because defaults and dependencies evolve between kernel releases.
- Treat the audit script in Lesson 04 as a metadata aid, not proof that the source tree builds or that a driver works.

No claim is made that the Kconfig/Kbuild behavior was executed against a real v7.4-rc4 checkout in this publication; the helper was syntax-checked and tested against a minimal mock fixture only.


## Lesson 05 — Module runtime, dynamic debug and asynchronous teardown

The v7.4-rc4 source entry points for this investigation are:
- [module_init()/module_exit() declarations](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/module.h)
- [Module loading and init invocation](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/module/main.c)
- [Parameter operations and sysfs handling](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/params.c)
- [Dynamic debug HOWTO](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/admin-guide/dynamic-debug-howto.rst)
- [Workqueue API and cancellation semantics](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/core-api/workqueue.rst)

### Repeatable history investigation

Use a full source checkout pinned to v7.4-rc4:

    git log --follow --oneline v7.4-rc4 -- include/linux/module.h
    git log --follow --oneline v7.4-rc4 -- kernel/module/main.c
    git log --follow --oneline v7.4-rc4 -- kernel/params.c
    git log --follow --oneline v7.4-rc4 -- Documentation/admin-guide/dynamic-debug-howto.rst
    git blame v7.4-rc4 -- include/linux/moduleparam.h

Then inspect the full commit and review discussion for any design change you use in a lesson. Do not infer the original motivation from a symbol name or a commit subject alone.

### Documented behavior versus interpretation

- **Documented/API behavior:** module initialization returns success or a negative errno; a failed init path must unwind its own acquisitions. Dynamic debug can select callsites at runtime when the kernel is configured for it. Workqueue cancellation APIs provide synchronization needed before freeing callback state.
- **Observed in the lab source:** the delayed-work callback owns no independent allocation; it references the context and copied string. The exit path sets a stop flag, drains the delayed work synchronously and only then frees those objects.
- **Engineering interpretation:** a module's code lifetime, a driver object's lifetime and a DMA buffer's hardware ownership are separate contracts. A module reference does not quiesce a bus master, and a successful cancel_delayed_work_sync() does not prove firmware stopped accessing shared memory.

This is a lifecycle case study, not a claim about a particular vendor's proprietary hardware implementation.


## Lesson 05 — Three concrete runtime-lifetime fixes in mainline

### Built-in parameter callback ran before the allocator

[NVMe PCI commit ed75528c095bc7760a3cf34a3f3e2ec7a59eb285](https://github.com/torvalds/linux/commit/ed75528c095bc7760a3cf34a3f3e2ec7a59eb285) documents an early boot hang when the built-in NVMe driver parsed nvme.quirks using module_param_cb(). The parameter callback ran at initcall level -1, before memory allocators were initialized. The fix used device_param_cb() to defer parsing to device initcall level 6.

- **Documented fact:** the commit message states the callback timing, the allocator dependency and the changed parameter macro.
- **Engineering lesson:** built-in parameter parsing can be earlier than normal driver probe. A setter that allocates memory or assumes device infrastructure must be placed in an appropriate initialization phase.

### Debug logging dereferenced a gadget after unbind

[USB gadget commit 018e3bf95e6360b7d528d3094b5302fb4e2fbed0](https://github.com/torvalds/linux/commit/018e3bf95e6360b7d528d3094b5302fb4e2fbed0) explains that the gadget pointer was cleared during unbind while the net_device remained alive. The old composite logging macros dereferenced the gadget device and could crash in eth_stop(); the fix changed to netdev_*() logging tied to the surviving network-device object.

- **Documented fact:** the commit includes the failure call trace and the macro replacement.
- **Engineering lesson:** a debug log call has pointer-lifetime requirements. Enabling dynamic debug can expose a dereference hidden in a logging macro.

### Atomic context cannot wait for delayed work

[fbcon commit b3a3f241dace3d4a775e1401b53a3f6d304534f0](https://github.com/torvalds/linux/commit/b3a3f241dace3d4a775e1401b53a3f6d304534f0) separates nonblocking cancellation from synchronous cancellation. A console cursor path could execute in atomic context, where cancel_delayed_work_sync() may sleep. The synchronous variant remains in release, where the callback must be finished before its context is freed.

- **Documented fact:** the commit message gives the atomic call path and the DEBUG_ATOMIC_SLEEP report.
- **Engineering lesson:** choose cancellation by both lifetime requirement and execution context. Nonblocking cancel alone does not prove callback completion; use a later sleepable teardown point to synchronize before freeing memory.



## Memory allocator archaeology: object caches, page allocation and reclaim

### The original object-cache motivation

Jeff Bonwick's 1994 USENIX paper describes the SunOS 5.4 slab allocator. The design retained reusable, typed objects instead of repeatedly constructing and destroying common kernel structures. The paper discusses object caches, cache coloring, reclaim under memory pressure and allocator statistics/debugging.

The 2001 Magazines and Vmem paper explains why per-CPU caching was added to scale allocation across many processors and reduce global-lock contention. This is design history for the broader slab family; do not imply Linux copied every Solaris implementation detail unchanged.

### Linux 7.4 slab update

The [slab-for-7.4 merge](https://github.com/torvalds/linux/commit/7a78f7ba9f24962c8033e0152bf34a7903ff61ba) includes:
- a SLUB sheaf-prefill improvement that allows a barn to retain a partial sheaf and refill it, with the pull report citing a 24% will-it-scale mmap1 throughput improvement on the maple_node cache;
- restored memory-cgroup accounting for IPC msg_msg allocations on the fallback path when CONFIG_SLAB_BUCKETS=n;
- a kmem_cache_destroy() versus slab_late_init() race fix;
- explicit rejection of unsupported oversized kmalloc() requests.

**Engineering interpretation:** allocator performance, memory accounting and allocator-object lifetime are coupled. A faster fast path is not acceptable if fallback paths lose cgroup accounting or teardown races access uninitialized allocator metadata. The reported 24% is a specific benchmark/cache result, not a general system-wide speedup.

### Linux 7.4-rc5 core-MM fixes

The [core MM merge 1f1b6ca3145a1179d88b0033a871e4e187da0d4e](https://github.com/torvalds/linux/commit/1f1b6ca3145a1179d88b0033a871e4e187da0d4e) fixes folio-lock deadlocks in splitting/migration, stale TLB/data visibility cases, MADV_FREE behavior and device-memory migration lifetime bugs. The pull request explicitly says the changes had spent several days in linux-next and had received review before being sent for the release-candidate cycle.

**Engineering interpretation:** allocation/free correctness does not end at kfree(). Page migration, reverse mapping, TLB invalidation and device memory can extend the lifetime/visibility contract beyond the original CPU pointer.

### Facts versus interpretation

- **Documented facts:** use the exact USENIX papers and merge messages linked above.
- **Observed implementation:** inspect the v7.4-rc4 allocator source and the exact commit diffs.
- **Interpretation:** allocator scalability, memcg accounting, reclaim recursion and object lifetime are recurring system-level constraints. Validate this interpretation against more than one subsystem and release.


## Lesson 07 — Object lifetime, RCU, workqueues and error unwinding

### io_uring: full object teardown after an RCU grace period

The [io_uring RCU-work fix a905ff43362e80590e54f6ea9ddb2d1a998aa01c](https://github.com/torvalds/linux/commit/a905ff43362e80590e54f6ea9ddb2d1a998aa01c), included in the [mainline io_uring pull 7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046](https://github.com/torvalds/linux/commit/7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046), explains why a final `kfree_rcu(ctx)` was not enough. Task-work add paths could still inspect ring fields while the exit work freed those members before the outer context. The change queues the whole `io_ring_ctx_free()` as RCU work so member teardown occurs after the grace period and in process context.

- **Documented fact:** the commit message names the RCU readers, fields accessed and the process-context requirement.
- **Engineering interpretation:** defer the complete teardown when readers can access members, not merely the final outer allocation. This is relevant to lockless registries, firmware contexts and device queues.

### io_uring: cached task references must be drained on every exit path

The [cached task-reference fix 6f3159575d53eed2d54557aef2dd8373da7c0208](https://github.com/torvalds/linux/commit/6f3159575d53eed2d54557aef2dd8373da7c0208) documents a task_struct leak when an exiting task released a ring with an in-flight request. A cancellation path could put a task reference into a cache after the ordinary task-exit path had already performed its final cache drain.

- **Documented fact:** the commit traces do_exit(), io_uring cancellation, ring release and the cached-reference path.
- **Engineering interpretation:** reference batching is safe only when every completion, error, close and exit path has a final drain boundary.

### Workqueue drain: chained work can land on another pool-workqueue

The [workqueue fix 94ff2d27fafad99c63c02ab4d7e148c37e4eff80](https://github.com/torvalds/linux/commit/94ff2d27fafad99c63c02ab4d7e148c37e4eff80) explains that a drain pass could observe pwq A as empty, then miss work queued there by a running item on pwq B. Destruction could then see a busy queue and leak it. The fix tracks started work and rechecks the pwqs.

The [maintainer pull report](https://lkml.iu.edu/hypermail/linux/kernel/2611.3/04049.html) describes the reproducer and the workqueue destruction failure.

- **Documented fact:** the commit reports reproducers for self-requeueing work on an unbound queue and chained queueing across per-CPU queues.
- **Engineering interpretation:** a drain is only a lifetime boundary after producers are controlled and the exact chained-work semantics are satisfied.

### SCMI device destroy and device-reference ownership

The [SCMI device lifetime fix 6abe8fe36b29ff51d1a42c2f338972883f4751a5](https://github.com/torvalds/linux/commit/6abe8fe36b29ff51d1a42c2f338972883f4751a5) explains that a child-device lookup dropped its reference before the destroy path finished using the pointer. It also delays reuse of a bus ID until sysfs removal is complete and centralizes release to avoid a double free.

- **Documented fact:** the commit message describes the premature put, concurrent unregister and bus-ID reuse window.
- **Engineering interpretation:** lookup references must remain held for the entire operation that consumes the object, not merely until the pointer is returned.

### Facts versus interpretation

- **Documented facts:** use the linked upstream commit messages and pinned v7.4-rc4 kref/devres/workqueue documentation.
- **Observed lab implementation:** one kref belongs to the module owner and one to delayed work; cancellation returning true means the cancel path owns the pending-work put, while a running callback owns its own put.
- **Engineering interpretation:** memory lifetime, code lifetime, publication lifetime and device DMA ownership are independent contracts. Every one must be closed before teardown completes.
