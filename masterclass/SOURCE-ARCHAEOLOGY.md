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
