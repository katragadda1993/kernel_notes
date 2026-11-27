# Upstream Merge and Kernel Trend Watch

This is the living engineering intelligence stream for the masterclass. Its purpose is to teach how the kernel is changing and what those changes mean to driver and SoC engineers.

## Sources to inspect on every run
1. [kernel.org releases and mainline status](https://www.kernel.org/)
2. [Mainline Git history](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/)
3. [linux-next integration tree](https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git/)
4. [Kernel development process](https://docs.kernel.org/process/2.Process.html)
5. [Kernel mailing-list archive](https://lore.kernel.org/)
6. Subsystem trees and pull requests for the topics relevant to the current lesson.
7. [Kernel Planet](https://planet.kernel.org/) and LWN for context, not as substitutes for source evidence.

## Classify every finding
- **Merged mainline:** accepted into Linus's tree; include commit/tag and diff.
- **linux-next:** integrated for testing but not yet merged into mainline.
- **Proposed/review:** posted to mailing lists or subsystem trees; may change or be rejected.
- **Released/stable:** identify the first released kernel containing it and any backport status.
- **Trend interpretation:** a pattern inferred from multiple changes or discussions; label it as analysis, not fact.

Never describe linux-next or a patch series as a released feature. Record the date checked and exact revision.

## Per-run workflow
1. Inspect repository progress and avoid repeating completed lessons.
2. Check current mainline release/RC, latest stable/LTS releases and linux-next snapshot.
3. Review new merge commits and subsystem pull requests since the previous recorded checkpoint.
4. Filter for high-value changes: new APIs, changed driver frameworks, architecture, memory, scheduler, DMA/IOMMU, PCI, audio, firmware/IPC, power, networking, storage, security, Rust, BPF and testing.
5. Select the most educational changes; do not produce a noisy commit dump.
6. For each selected change, explain the old behavior, new behavior, motivation with evidence, call path/source diff, hardware implications, compatibility/backport risks and validation.
7. Create a concise trend briefing and, when substantial, a full lesson with a runnable lab.
8. Add at least 10 non-duplicate interview questions to the bank for every full lesson.
9. Update a dated checkpoint so the next run can compare new activity against it.

## Trend dashboard categories
- Core architecture, scheduler and memory management
- Rust for Linux and safe driver abstractions
- eBPF, BTF, tracing and sched_ext
- DMA, IOMMU, PCIe and device assignment
- Firmware, remoteproc, RPMsg, mailbox and SCMI
- Audio, media, DSP and accelerator drivers
- Networking, XDP and high-throughput I/O
- Storage, filesystems and NVMe
- Power, thermal and energy-aware scheduling
- Security, hardening and vulnerability response
- Kernel testing, CI, regression and AI-assisted engineering workflows

## Initial trend baseline — checked 2026-11-28
The live kernel release index reported mainline 7.4-rc4, stable 7.3.6 and linux-next snapshot next-20261127. Treat this as a dated snapshot, not a permanent version statement.

Early signals worth following:
- **sched_ext/BPF scheduling:** continued work on composable BPF schedulers, CPU grants, nested scheduling and hardware/topology-aware placement.
- **Rust for Linux:** growth of driver-facing abstractions, including PCI-related and synchronization/DMA-adjacent support; track actual merged scope rather than assuming broad driver conversion.
- **Memory and virtualization:** working-set tracking, memory-attribute monitoring and huge-page handover work are examples of changes that affect VM and memory-management behavior.
- **Networking scalability:** ongoing work to reduce global lock dependencies, improve per-network-namespace behavior, batching and queue utilization.
- **AI-assisted kernel development:** recent discussion suggests generated fixes are entering review and merge workflows, but acceptance still depends on normal evidence, testing, review and maintainer standards.

This baseline is a starting hypothesis for future comparison. Each item must be revalidated against mainline commits and subsystem sources before being taught as current.

## How to decide what is a real trend
A single commit is a change, not necessarily a trend. Prefer evidence across multiple releases, subsystem pull requests, conference discussions or maintainer statements. Explain who benefits, which workloads/hardware are affected, and what an engineer should learn next.

## Required output
- Date and kernel revisions checked
- 3–8 meaningful upstream changes, with links
- 2–5 trend observations, each with evidence and confidence
- Driver/SoC engineering impact
- One recommended deep-dive lesson or lab
- Interview questions added (count and link)
- Gaps and unverified items


## Checkpoint 2026-11-28 — post-v7.4-rc4 mainline review

### Release and tree status

- The kernel.org release index lists mainline **7.4-rc4** (2026-11-22) and stable **7.3.6** (2026-11-23).
- The same index lists longterm **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275** (all shown with 2026-11-23 release dates).
- The latest linux-next snapshot is **next-20261127**, published 2026-11-27; Gitiles identifies checkpoint commit **e5aad2bdbbee783611d32599f89cea3f12f9593d**. This matches the previous checkpoint; no newer 2026-11-28 linux-next snapshot was observed.
- Linus's mainline **master** branch has accepted several pull requests labelled for 7.4-rc5. The release tag visible in the kernel.org index remains 7.4-rc4. Treat the merged master-branch commits below as mainline-merged but not yet part of a tagged 7.4-rc5 release.

### Selected mainline changes

1. **Core MM: folio locks, TLB ordering and device-memory migration**  
   Classification: merged mainline, post-v7.4-rc4.  
   Merge: [1f1b6ca3145a1179d88b0033a871e4e187da0d4e](https://github.com/torvalds/linux/commit/1f1b6ca3145a1179d88b0033a871e4e187da0d4e), 2026-11-27.  
   The pull request describes folio-lock deadlocks during split/migration, stale TLB entries with delayed rmap handling, MADV_FREE data-loss/stale-swap cases and device-memory migration lifetime bugs including anon-rmap use-after-free. The maintainer says the fixes had several days in linux-next after review.  
   Engineering impact: accelerator/device-memory migration, page-table/TLB correctness and memory lifetime need to be considered together. A driver that pins or migrates device-backed pages must respect the MM contract; a DMA mapping is not a substitute for page lifetime or CPU TLB synchronization.

2. **arm64: page-table barriers and break-before-make**  
   Classification: merged mainline, post-v7.4-rc4.  
   Merge: [822ba3a89c7896b26e4beb70e2ebffb881924dfe](https://github.com/torvalds/linux/commit/822ba3a89c7896b26e4beb70e2ebffb881924dfe), 2026-11-27.  
   The pull includes missing barriers and break-before-make fixes for fixmap and early ID-map updates, ISBs after mapping the FDT and remapping a KPTI page-table slot, plus late module PLT/ADRP veneer writes routed through text-poke because the module PLT section is read-only at that point.  
   Engineering impact: page-table writes, TLB visibility and instruction-stream synchronization are distinct ordering requirements. Never assume a page-table store alone makes a new translation or executable patch immediately safe on every CPU.

3. **tg3: a coherent DMA mask becomes best-effort on affected hardware**  
   Classification: merged mainline, post-v7.4-rc4; not yet a tagged release.  
   Fix: [686b8dbcedf8773dc505f9f315a0551c0bca70a3](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/686b8dbcedf8773dc505f9f315a0551c0bca70a3), included by [net-7.4-rc5 merge 9437ee8b6cf607e883b36e44bd755968daaebd48](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/9437ee8b6cf607e883b36e44bd755968daaebd48).  
   A 31-bit coherent mask used as a BCM57766 workaround could be rejected on a direct-DMA system with memory above 2 GiB and no CONFIG_ZONE_DMA. Making that rejection fatal prevented the device from probing. The fix treats the narrow mask as best-effort, warns and falls back to the previous mask if unsupported, and warns when a status block lands at the documented problematic address pattern. The [patch discussion](https://lists.openwall.net/netdev/2026/11/08/62) explains the platform conditions.  
   Engineering impact: DMA mask negotiation is a platform capability contract, not merely a number copied from a datasheet. Test coherent and streaming masks, direct DMA and IOMMU modes, physical-memory ranges and the hardware erratum behavior separately. The fallback preserves probe availability but does not magically preserve the erratum workaround on platforms that reject the mask.

4. **dpaa2-eth: a cleanup warning exposed a real AF_XDP ownership bug**  
   Classification: merged mainline.  
   Commit: [25faec9ce61c01fb45abf812a222c5b43c3cd5c5](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/25faec9ce61c01fb45abf812a222c5b43c3cd5c5), 2026-11-25.  
   Linus explains that an objtool stack-state warning led to simplifying shared cleanup. That made a separate bug visible: a loop freeing buffers that had not been mapped incremented the same variable used to count successfully mapped buffers. The later path therefore treated freed/unmapped buffers as mapped and released them to hardware. The commit explicitly says objtool did not prove the bug; code cleanup exposed it.  
   Engineering impact: keep ownership counters separate from cleanup-loop indices. Model descriptor/buffer state transitions explicitly: allocated → mapped → device-owned → completed → unmapped/freed. Error unwind must release only resources actually acquired.

5. **USB/Thunderbolt: lifecycle, bounds and error-path fixes**  
   Classification: merged mainline, post-v7.4-rc4.  
   Merge: [7641bffc3721f0d1650e52935369ec7f7bf84126](https://github.com/torvalds/linux/commit/7641bffc3721f0d1650e52935369ec7f7bf84126), 2026-11-27; 91 commits.  
   The merge message lists gadget, DWC2/DWC3, Type-C, USBIP, USB storage and Thunderbolt fixes. Examples in the merge include a dangling xHCI pointer on initialization failure, use of a destroyed mutex during gadget exit, a free of static platform data on remove, and endpoint bounds checking. The maintainer notes that some fixes came from syzbot or LLM-found reports with reproducers and had spent a week in linux-next.  
   Engineering impact: probe/remove lifetime, endpoint bounds, timeout handling and reproducible fuzzing reports remain high-value driver work. The merge does not mean every LLM-generated patch is trusted; the reproducer, review and integration testing are the evidence.

6. **sched_ext: scheduler state transitions and BPF JIT capability checks**  
   Classification: merged mainline.  
   Merge: [f441d24349a3fb7f0b0f8f76f05686e448fd0b10](https://github.com/torvalds/linux/commit/f441d24349a3fb7f0b0f8f76f05686e448fd0b10), 2026-11-25.  
   The merge fixes races while enabling/disabling sched_ext, an RT starvation path during fallback to fair scheduling, BPF schedulers requiring private stacks on JITs that cannot provide them, and kfunc calls from callbacks where task movement is unsafe. It adds sched_ext selftests.  
   Engineering impact: BPF programmability does not remove scheduler concurrency invariants. Feature support must be checked against architecture/JIT capabilities, and enable/disable must be a carefully ordered state transition.

7. **Tracing and ext4 validation**  
   Classification: merged mainline.  
   Tracing merge: [2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/2bad4ba5bd4ed7075c7e874cd30c52e07a74c8cc), 2026-11-25. It prevents KASAN from incorrectly checking certain stack reads and zeroes faulted probe dereference fields instead of leaving stale ring-buffer data.  
   Ext4 post-merge fixes: [df99e82b36ff3c243ec3a06afc0ccaa4866a5a41](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/df99e82b36ff3c243ec3a06afc0ccaa4866a5a41), 2026-11-26. The merge fixes KUnit registration/test failures and a fast-commit/inline-data test failure after a prior ext4 pull.  
   Engineering impact: observability tools must be safe around faults and instrumentation, and test suites themselves need regression coverage after subsystem merges.

### Trend observations and confidence

- **Driver lifecycle and resource ownership — high confidence as a recurring engineering theme.** tg3 DMA-mask negotiation, dpaa2 AF_XDP ownership and USB remove/init failure fixes all reinforce explicit resource state, platform capability checks and unwind correctness.
- **MM/TLB and device-memory lifetime — high confidence as a current correctness hotspot.** The rc5 MM pull contains multiple fixes in folio migration/splitting, stale TLB handling and device-memory migration. This is especially relevant to accelerators and IOMMU/DMA integration.
- **AI-assisted bug discovery and validation — medium confidence, emerging practice.** USB maintainers explicitly cite LLM-found reports with reproducers; ext4 adds KUnit coverage to validate AI-generated patches. This is evidence of AI entering the engineering workflow, not evidence that unreviewed generated patches are acceptable.
- **BPF/sched_ext portability and lifecycle — medium confidence, active feature-hardening.** Recent fixes show architecture-specific JIT capabilities and concurrent enable/disable state are practical constraints for programmable scheduling.
- **arm64 translation and instruction ordering — medium confidence, current fix cluster.** The recent pull groups several barrier, break-before-make and text-poke fixes. Track the exact architecture contracts rather than generalizing one pull into a permanent trend.

### What changed versus the prior checkpoint

- linux-next remains at next-20261127, so there is no new integration snapshot to compare yet.
- The previous checkpoint recorded v7.4-rc4 as the latest release tag. This checkpoint adds the mainline master-branch 7.4-rc5 subsystem pulls now visible on Nov25–27, while retaining the distinction that the published release tag is still v7.4-rc4.
- The next run should compare against commit e5aad2bdbbee783611d32599f89cea3f12f9593d for linux-next and the latest recorded mainline commit links above. Do not re-report these as new unless later commits change the status or implementation.
