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
