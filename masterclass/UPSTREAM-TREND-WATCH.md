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
