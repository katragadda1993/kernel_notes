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
