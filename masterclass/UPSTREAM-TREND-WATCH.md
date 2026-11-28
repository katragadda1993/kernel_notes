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
   The earlier ext4 pull [4dce97c6c265544342e716e62fd24bbb232590c5](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/4dce97c6c265544342e716e62fd24bbb232590c5), 2026-11-24, added KUnit tests; its maintainer explicitly said those tests make it safer to validate AI-generated patches. Ext4 post-merge fixes: [df99e82b36ff3c243ec3a06afc0ccaa4866a5a41](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux.git/+/df99e82b36ff3c243ec3a06afc0ccaa4866a5a41), 2026-11-26. The merge fixes KUnit registration/test failures and a fast-commit/inline-data test failure after a prior ext4 pull.  
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


## Checkpoint 2026-11-28 04:14 IST — Lesson 02 run

### Release and tree status

- The current kernel.org search index still reports mainline **7.4-rc4** and stable **7.3.6**. It lists longterm releases including **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275**. See [kernel.org](https://www.kernel.org/).
- The current [kernel.org release index](https://www.kernel.org/) lists **next-20261127**, dated 2026-11-27, agreeing with the previous checkpoint. The direct Gitiles browse endpoint returned a cached next-20261120 page and its next-20261127 browse link returned HTTP 403; use the kernel.org index as the authoritative tag/date for this checkpoint. The exact tag commit hash was not retrievable in this run.
- The current mainline repository index still lists [9437ee8b6cf607e883b36e44bd755968daaebd48](https://github.com/torvalds/linux/commit/9437ee8b6cf607e883b36e44bd755968daaebd48) as the master branch head, the net-7.4-rc5 pull from 2026-11-26. This was already recorded in the previous checkpoint. The published release index still says 7.4-rc4; merged mainline work is not automatically a released 7.4-rc5.

### Material delta

No new mainline merge after the previous checkpoint's latest confirmed mainline set was established in this run. The previously recorded core-MM, arm64, networking, USB, tracing, ext4, sched_ext and DMA/driver changes remain relevant. Do not relabel them as newly merged.

### Supplemental mainline change now highlighted

**Trusted keys and TEE session lifetime**  
Classification: merged mainline during the 7.4-rc5 development cycle; already in the history leading into the previous checkpoint, not a new delta for this run.  
Commit: [5888d706750d0e5dce374c01d2c656bbfe2045be](https://github.com/torvalds/linux/commit/5888d706750d0e5dce374c01d2c656bbfe2045be), 2026-11-27.  
The pull fixes trusted-key backend static-call initialization before backend init, synchronizes TEE session lifetime against device removal, disables sysfs bind/unbind for the affected TEE device path, propagates CAAM option parse errors, adds user-buffer bounds checks in public-key keyctl operations and uses kfree_sensitive() for temporary sensitive buffers.  
Driver/SoC impact: firmware-backed sessions are resources with a lifetime that must be serialized against device unbind/remove. This is directly relevant to TEE, secure key storage, mailbox-backed firmware services and any driver exposing a session through sysfs. It reinforces the already observed lifecycle trend alongside USB remove-path and DMA ownership fixes.

### Lesson connection: early boot ordering and arm64 translation

The previously recorded [arm64 fixes merge 822ba3a89c7896b26e4beb70e2ebffb881924dfe](https://github.com/torvalds/linux/commit/822ba3a89c7896b26e4beb70e2ebffb881924dfe) includes an ISB after mapping the FDT in the initial identity map, break-before-make handling for fixmap entries and barrier changes. This is a concrete example of why boot-time page-table visibility, TLB maintenance and instruction synchronization must be reasoned about separately. It is merged mainline work in the 7.4-rc5 development cycle, not a claim that a 7.4-rc5 release tag exists.

### Trend observations

- **Driver and firmware session lifetime — high confidence as a recurring correctness theme.** The TEE session/remove synchronization, USB teardown fixes and AF_XDP buffer-ownership fix are distinct subsystems but share a common engineering requirement: state and ownership must remain valid across asynchronous work and teardown.
- **Boot-time translation ordering — a specific recent fix cluster, not yet a broad trend claim.** The arm64 pull contains multiple page-table, TLB and ISB corrections. Use it to teach architecture-specific ordering; do not generalize beyond the evidence.
- **AI-assisted patch validation — emerging practice, medium confidence.** The ext4 pull's KUnit expansion and maintainer explanation remain a concrete example. No new evidence in this run changes the prior assessment.
- **linux-next integration snapshot — no new delta.** The kernel.org index still lists next-20261127, the same dated snapshot recorded previously. No newer linux-next tag was listed at the time checked; the tag's commit hash remains unverified because the browse endpoint returned 403.

### Next checkpoint requirements

- Recheck kernel.org, the mainline master ref and the linux-next tag/commit when the mirror is fresh.
- Compare against mainline merge 9437ee8 and the already-recorded arm64/MM merges; do not duplicate them.
- Follow the TEE trusted-session lifetime fix through the source diff and related driver tests if a relevant new pull appears.
- Continue the curriculum with Lesson 03; no upstream-derived lesson is justified solely by the lack of new merge activity.


## Checkpoint 2026-11-28 05:15 IST — mainline merge-list reconciliation

### Release and integration status

- [kernel.org](https://www.kernel.org/) still lists mainline **7.4-rc4** (2026-11-22) and stable **7.3.6** (2026-11-23). Longterm releases shown include **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275**.
- The latest linux-next tag listed remains **next-20261127** (2026-11-27). No next-20261128 tag is listed on the kernel.org index. The exact linux-next commit was not independently retrieved in this run.
- The latest merge returned by the mainline commit search was [f4585067150b85e33f60876149a6e742928da75e](https://github.com/torvalds/linux/commit/f4585067150b85e33f60876149a6e742928da75e), the thermal-7.4-rc5 pull. **Timestamp reconciliation:** the selected merge commits below are dated 2026-11-27 17:42–20:41 UTC (23:12–02:11 IST). Those timestamps are earlier than the previous 04:14 IST checkpoint. Therefore this is a correction/completion of the previous checkpoint's incomplete merge list, not evidence that these commits landed after 04:14. The previous checkpoint had recorded 9437ee8 as the latest mainline merge but missed these already-present commits. Kernel.org still publishes 7.4-rc4; do not call these released 7.4-rc5 features.

### Selected mainline merges newly captured in this reconciliation

1. **ALSA/ASoC: PCM buffer lifetime, locking and stream-state fixes**  
   Classification: merged mainline; not yet in a new published release tag.  
   Merge: [16a4afe366a22507ccb7b3302f956e6404aeb4b6](https://github.com/torvalds/linux/commit/16a4afe366a22507ccb7b3302f956e6404aeb4b6), sound-7.4-rc5, 2026-11-27.  
   The pull summary lists a potential ABBA deadlock in snd_pcm_action_nonatomic(), prevention of hw_params/hw_refine changes while PCM buffer access is active, a buffer-access gate refactor using scope-based cleanup, timer-flag locking, and USB-audio URB list corruption after stop. ASoC fixes include FSL ASRC/EASRC memory safety, pair/context ownership on repeated hw_params, and FIFO drain bounds. It also includes Tegra268 ADMAIF driver and binding additions.  
   Engineering impact: audio buffer lifetime must be protected across userspace read/write/mmap and DMA synchronization. Repeated hw_params and stop/close transitions need explicit state and ownership rules. A PCM period buffer cannot be reallocated while a concurrent access/sync path still holds it.

2. **IOMMU: cache coherency, hard-IRQ runtime PM and failed domain deletion**  
   Classification: merged mainline; not yet in a new published release tag.  
   Merge: [8c8611425a367d250339cdb05aa898971d35abdb](https://github.com/torvalds/linux/commit/8c8611425a367d250339cdb05aa898971d35abdb), 2026-11-27.  
   AMD IOMMU v2 page tables are marked DMA-incoherent on affected Cezanne hardware so CPU cache lines are flushed. VeriSilicon IOMMU fixes include avoiding pm_runtime_resume_and_get() from a hard IRQ path, using pm_runtime_get_if_active(), balancing runtime-PM references, flushing the TLB on resume, correcting domain-switch locking and unwinding clocks when IRQ lookup fails. Hyper-V IOMMU retains a domain's page tables and ID if the hypervisor fails to delete the domain, because the hypervisor may still access them.  
   Engineering impact: the IOMMU page tables themselves have a coherency contract; this is distinct from the payload DMA mapping. Runtime PM callbacks must not sleep in hard IRQ context. If hardware/firmware refuses to relinquish a translation domain, freeing the backing page tables can become a use-after-free visible to the device.

3. **GPIO: chip removal, IRQ ownership and resume output ordering**  
   Classification: merged mainline; not yet in a new published release tag.  
   Merge: [3852adbb01116275eadf41c67af5ef68abe9755a](https://github.com/torvalds/linux/commit/3852adbb01116275eadf41c67af5ef68abe9755a), gpio fixes for v7.4-rc5, 2026-11-27.  
   The pull fixes IRQ and label lifetime around gpiochip removal, avoids sleeping inside an SRCU read-side section in a GPIO read path, handles pending requests during chip removal, and fixes driver-specific use-after-free/resource issues. The PCA953x resume path now restores output levels before changing direction to outputs, avoiding a transient power-on output level.  
   Engineering impact: IRQ numbers and descriptors can be reused after device removal, so teardown must stop event sources while the old chip/request relationship is still serialized. Register restore order is electrically observable: direction-before-output can momentarily drive an unintended level.

4. **CPU frequency and system resume: lock ordering and hotplug refresh**  
   Classification: merged mainline; not yet in a new published release tag.  
   Merge: [2cf42c49089d09e6b2a1dc35186f15721fe18c9c](https://github.com/torvalds/linux/commit/2cf42c49089d09e6b2a1dc35186f15721fe18c9c), pm-7.4-rc5, 2026-11-27.  
   The intel_pstate changes introduce a dedicated lock for the power-control MSR update path to avoid lockdep complaints on system resume, serialize a no_turbo update against CPU online, and refresh cpuinfo.max_freq when a CPU comes online.  
   Engineering impact: runtime configuration, system resume and CPU hotplug can race. A lock chosen for one call path may be held in a different order on resume; policy values cached while a CPU is offline may be stale when it returns.

5. **Thermal sysfs: serialize access to thermal-zone parameters**  
   Classification: merged mainline; latest merge observed in this checkpoint.  
   Merge: [f4585067150b85e33f60876149a6e742928da75e](https://github.com/torvalds/linux/commit/f4585067150b85e33f60876149a6e742928da75e), thermal-7.4-rc5, 2026-11-27.  
   The change adds thermal-zone locking in sustainable_power show/store and related trip/hysteresis attribute handlers before accessing tzp. The pull description identifies a possible NULL dereference in the thermal sysfs interface.  
   Engineering impact: sysfs callbacks can race with device/zone reconfiguration or teardown. A pointer check without a lifetime/serialization guarantee is not sufficient.

6. **Staging: removal of an unused VME user driver**  
   Classification: merged mainline; not yet in a new published release tag.  
   Merge: [6cce17ffe54a9fdcadbadfd59c00de7ac2cba25d](https://github.com/torvalds/linux/commit/6cce17ffe54a9fdcadbadfd59c00de7ac2cba25d), staging-7.4-rc5, 2026-11-27.  
   The maintainer removed vme_user, stating there were no users and that AI-generated security-bug reports were creating review churn despite lack of real use. The pull says the removal had been in linux-next without reported issues.  
   Engineering impact: long-term maintenance cost, real hardware usage and verifiable reports matter. Static-analysis/LLM findings should be triaged and reproduced; they should not automatically force changes to unmaintained code.

### Trend observations

- **Resource lifetime and synchronization — high confidence, recurring across subsystems.** ALSA buffer access, GPIO IRQ/chip removal, IOMMU domain deletion and thermal sysfs locking are separate fixes with a common requirement: protect an object for the entire operation, not only at pointer lookup.
- **IOMMU and DMA coherency — high relevance, hardware-specific evidence.** The AMD Cezanne page-table flush fix shows that IOMMU translation structures can have their own cache-coherency requirements. This is not the same as synchronizing the payload buffer.
- **Runtime PM must respect execution context — high confidence.** The VeriSilicon hard-IRQ change and CPU-frequency resume fixes show that PM callbacks, locks and CPU hotplug have to be analyzed together.
- **Audio pipeline memory/state correctness — medium-high confidence.** ALSA PCM buffer gating, USB-audio URB lifecycle and ASoC ASRC ownership fixes reinforce that stream stop/reconfigure/close must serialize with active buffer users and asynchronous completions.
- **AI-assisted code triage and project sustainability — emerging practice.** A maintainer explicitly cited AI-generated false-positive churn in the VME removal discussion. Treat AI reports as hypotheses requiring a reproducer, code-path proof and maintainer review.

### Lesson connection

Lesson 03 explains why Kbuild, type annotations, object layout and link order are runtime concerns. The ALSA and IOMMU changes are good follow-up reading:
- For ALSA, trace buffer-access lifetime and hw_params/hw_free serialization in the sound merge diff.
- For IOMMU, compare CPU cache maintenance for translation tables with DMA API synchronization for payload buffers.
- For GPIO, trace IRQ/request lifetime from userspace line request through gpiochip removal.

### Next checkpoint

- Use mainline merge [f4585067150b85e33f60876149a6e742928da75e](https://github.com/torvalds/linux/commit/f4585067150b85e33f60876149a6e742928da75e) as the latest recorded merge baseline, along with the selected sound/IOMMU/GPIO/PM merges above. These commits were newly recorded here but predate the prior 04:14 IST checkpoint.
- Recheck kernel.org for a new mainline release tag and linux-next snapshot.
- Do not repeat these as new unless a later merge changes the implementation or status.


## Checkpoint 2026-11-28 06:18 IST — no material upstream delta

### Release and branch status

- [kernel.org](https://www.kernel.org/) continues to list mainline **7.4-rc4** and stable **7.3.6**. The indexed longterm releases include **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275**.
- The latest linux-next snapshot visible in the index remains **next-20261127**. I could not independently retrieve the exact linux-next tag object/hash in this run; do not infer it from a mirror of a different integration branch.
- The latest mainline merge baseline already recorded in the previous checkpoint remains **6cce17ffe54a9fdcadbadfd59c00de7ac2cba25d** (staging-7.4-rc5 VME driver removal). The current retrievable mainline mirror also shows that commit as its master head. A separate mirror exposes 9437ee8 (net-7.4-rc5) and its ext4 post-merge parent, but those commits were already reachable/known before this checkpoint; mirror timestamps/indexing differ. No later post-checkpoint mainline merge was independently verified.
- Kernel.org's published release tag remains 7.4-rc4. The 7.4-rc5 subsystem pulls described below are merged development-branch work, not a published 7.4-rc5 release.

### Upstream briefing

No new material delta was verified since the 05:15 IST checkpoint. Preserve the already captured changes rather than duplicate them:

1. **Merged mainline — ALSA/ASoC:** [sound-7.4-rc5 merge](https://github.com/torvalds/linux/commit/16a4afe366a22507ccb7b3302f956e6404aeb4b6) covers PCM buffer access lifetime/locking, USB-audio URB stop cleanup and ASoC repeated-hw_params ownership. Relevant to audio DMA buffers, stream teardown and XRUN-safe lifecycle.
2. **Merged mainline — IOMMU:** [IOMMU pull](https://github.com/torvalds/linux/commit/8c8611425a367d250339cdb05aa898971d35abdb) includes AMD IOMMU page-table cache coherency, runtime-PM execution-context corrections and safe retention of page tables when a hypervisor refuses domain deletion. Relevant to device-visible translation lifetime and SoC coherency.
3. **Merged mainline — GPIO:** [GPIO pull](https://github.com/torvalds/linux/commit/3852adbb01116275eadf41c67af5ef68abe9755a) fixes IRQ/request lifetime during chip removal and output restoration ordering during resume. Relevant to pin control and externally observable power sequencing.
4. **Merged mainline — PM/cpufreq:** [PM pull](https://github.com/torvalds/linux/commit/2cf42c49089d09e6b2a1dc35186f15721fe18c9c) addresses resume lock ordering and CPU-hotplug frequency refresh. Relevant to driver/firmware power-state coordination.
5. **Merged mainline — thermal:** [thermal pull](https://github.com/torvalds/linux/commit/f4585067150b85e33f60876149a6e742928da75e) adds serialization around thermal-zone parameter sysfs access to avoid a possible NULL dereference.
6. **Merged mainline — staging:** [VME driver removal](https://github.com/torvalds/linux/commit/6cce17ffe54a9fdcadbadfd59c00de7ac2cba25d) removes an unused driver after maintainer-reported maintenance/review churn.

The ext4 merge discussion is useful source archaeology, not a new merge in this checkpoint: [maintainer pull request and Linus review](https://lore.kernel.org/lkml/179557603037.190026.16193029745159421407.pr-tracker-bot@kernel.org/t/). The ext4 pull included a large KUnit regression-test addition for inode-migration fixes. Linus objected to adding a very large test/development batch during -rc stabilization and emphasized that new development/tests belong on a separate branch for the next merge window. The merge was nevertheless accepted after discussion. Teach this as release-management/review process, not as an endorsement of arbitrary AI-generated code.

### Trend observations

- **Object lifetime and teardown — recurring, high confidence:** sound PCM buffers, GPIO IRQ requests and IOMMU page tables all require ownership to persist until asynchronous users and hardware are quiesced.
- **IOMMU page-table coherency — hardware-specific but important:** page-table cache maintenance is separate from payload DMA synchronization. An IOMMU can itself be a non-coherent DMA client.
- **Runtime PM and execution context — recurring:** never call a sleeping resume path from hard IRQ context; reason about PM references, IRQ threading, lock order and CPU hotplug together.
- **Test growth and merge-window discipline — maintainer-backed process lesson:** KUnit regression coverage is valuable, but a large test/development series still needs appropriate timing, review and conflict management.
- **AI-assisted reports — emerging workflow, not a quality guarantee:** a generated finding must be reproduced, reduced, mapped to a real call path and reviewed. Duplicate/noisy reports can consume maintainer capacity.

### Next checkpoint

- Re-query kernel.org and mainline master; record exact tag/object hashes when accessible.
- Do not label any 7.4-rc5 pull as a released 7.4-rc5 feature until the release tag exists.
- Continue with Lesson 05; the current upstream delta does not justify replacing the sequential curriculum lesson with an unrelated trend lesson.


## Checkpoint 2026-11-28 07:15 IST — no post-checkpoint mainline delta

### Release and tree status

- The [kernel.org release index](https://www.kernel.org/) continues to list mainline **7.4-rc4** (2026-11-22) and stable **7.3.6** (2026-11-23). Indexed longterm releases include **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275**.
- The latest linux-next snapshot visible in the available index/search remains **next-20261127**. No next-20261128 result was found. Direct git.kernel.org access returned HTTP 403 in this run, so the exact current linux-next tag object/hash was not independently verified.
- The [GitHub mainline commit history](https://github.com/torvalds/linux/commits/master) and GitHub commit search still show **f4585067150b85e33f60876149a6e742928da75e** (thermal-7.4-rc5 pull, 2026-11-27 20:41:24 UTC) as the latest visible Linus commit. That timestamp is 2026-11-28 02:11 IST, earlier than the previous 06:18 IST checkpoint. No newer mainline merge was found after that checkpoint.
- The release tag remains 7.4-rc4. The already recorded 7.4-rc5-labelled subsystem pulls are merged development-branch commits, not a published 7.4-rc5 release.

### Upstream briefing

No new material delta was verified since the 06:18 IST checkpoint. Retain the previously captured ALSA/ASoC, IOMMU, GPIO, PM/cpufreq, thermal, staging, MM, arm64, USB and sched_ext changes as the current evidence set; do not re-label them as new in this run.

The latest mainline merge search was re-run against torvalds/linux and the current GitHub commits page. The latest visible merge remains the thermal pull above. The kernel.org index still reports the same release versions. The linux-next tag search did not surface a newer daily tag.

### Trend observations

- **Resource lifetime and synchronization — high confidence, recurring:** the previously recorded ALSA buffer, GPIO IRQ, IOMMU domain and thermal sysfs fixes continue to show the same object-lifetime requirement.
- **IOMMU/page-table coherency — hardware-specific:** retain the distinction between IOMMU translation-table cache maintenance and payload DMA synchronization.
- **Runtime PM and execution context — recurring:** retain the hard-IRQ sleep restriction and lock-order analysis from the VeriSilicon IOMMU and cpufreq changes.
- **AI-assisted bug triage — emerging workflow:** prior maintainer statements show both useful reproducer-backed reports and review churn. This remains an evidence/triage lesson, not a claim that generated patches are intrinsically reliable.

### Next checkpoint

- Compare the next run against mainline merge f4585067150b85e33f60876149a6e742928da75e and linux-next next-20261127.
- Recheck kernel.org for a new release tag and the linux-next refs/log for a new daily snapshot.
- Continue the sequential curriculum with Lesson 06 — Kernel Memory Allocation and GFP Context. No upstream change in this run justifies skipping the planned lesson.


## Checkpoint 2026-11-28 08:12 IST — memory merges reconciled; no confirmed post-07:15 delta

### Release and integration status

- The [kernel.org release index](https://www.kernel.org/) lists mainline **7.4-rc4** (2026-11-22), stable **7.3.6** (2026-11-23), and longterm **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275**. The published mainline release tag remains 7.4-rc4.
- The same index lists linux-next **next-20261127** (2026-11-27). No newer official daily snapshot was visible.
- Mainline commit views are not perfectly synchronized across mirrors. The prior checkpoint's f4585067 thermal merge remains the last recorded baseline in the lesson ledger. Current mirror views also expose the 7.4-rc5 development merges below. Direct git.kernel.org access was not available, so do not claim a verified new object hash for the live mainline tip.
- The merges below are development-branch mainline commits; they do not mean a published 7.4-rc5 release exists.

### Upstream briefing — memory-relevant merges reconciled

These merges were already present before the prior 07:15 IST checkpoint but were not fully described there. They are added here as a reconciliation, not falsely presented as newly merged after that checkpoint.

1. **Merged mainline — core MM, 7.4-rc5 development:** [1f1b6ca3145a1179d88b0033a871e4e187da0d4e](https://github.com/torvalds/linux/commit/1f1b6ca3145a1179d88b0033a871e4e187da0d4e). Fixes folio-lock deadlocks in split/migration paths, stale TLB/data visibility cases, MADV_FREE data/stale-swap issues, device-memory migration use-after-free/oops cases and an mm lifetime bug when copy_process() fails. The [maintainer pull request](https://lkml.iu.edu/hypermail/linux/kernel/2611.3/10608.html) states the changes had several days in linux-next after review. Hardware relevance: device-memory migration and page/folio lifetime are directly relevant to accelerators, HMM and shared-memory drivers. Do not backport isolated hunks without their locking/TLB/rmap dependencies.
2. **Merged mainline — MM hotfixes, 7.4-rc5 development:** [b312ada03437d5874a8dbac6b4a1d479c2af3da9](https://github.com/torvalds/linux/commit/b312ada03437d5874a8dbac6b4a1d479c2af3da9). The [maintainer summary](https://lore-kernel.gnuweeb.org/linux-mm/20261124224409.56e65d8ff1f72a1d485450ad%40linux-foundation.org/) describes swapcache/cluster races, hugetlb/TLB/migration fixes, list_lru/cgroup ID lifetime, MGLRU reparenting and memcg accounting corrections. It also reports a workload-specific swap allocation throughput improvement for an NVMe mysql/sysbench case. Hardware relevance: reclaim and cgroup accounting can affect I/O latency and memory pressure observed by device drivers. Benchmark figures are workload-specific.
3. **Merged mainline — SLUB/slab updates for 7.4:** [7a78f7ba9f24962c8033e0152bf34a7903ff61ba](https://github.com/torvalds/linux/commit/7a78f7ba9f24962c8033e0152bf34a7903ff61ba). The [maintainer discussion](https://lkml.iu.edu/hypermail/linux/kernel/2610.2/16280.html) documents the sheaf/barn prefill performance work, restored memcg accounting for IPC message allocations, a cache-destroy/bootstrap race fix and oversized-kmalloc rejection. The cited 24% result applies to will-it-scale mmap1 on the maple_node cache, not all allocations.
4. **Merged mainline — networking fixes, 7.4-rc5 development:** [9437ee8b6cf607e883b36e44bd755968daaebd48](https://github.com/torvalds/linux/commit/9437ee8b6cf607e883b36e44bd755968daaebd48). The pull covers Wi-Fi, Bluetooth, CAN and Ethernet fixes, including an mlx5e NULL-pointer write in a peer-flow error path and XFRM validation of non-IP packets. Hardware relevance: error-path correctness in high-throughput drivers still depends on pointer validation and packet-type invariants.

### Trend observations

- **Object/page lifetime is a recurring correctness boundary — high confidence.** SLUB cache destruction, folio migration, rmap/TLB invalidation and device-memory migration all require that an object/page not be reused while another CPU, device or subsystem can still reference it.
- **Memory accounting must survive allocator fallback — high confidence.** The SLUB/IPC fix demonstrates that a fallback from a specialized cache to general kmalloc must preserve memcg charging semantics.
- **Allocator throughput is increasingly optimized through local caching and batching — evidence-backed but workload-specific.** The sheaf/barn prefill report gives a concrete benchmark result. Measure the real object/cache/workload before claiming a driver benefit.
- **Reclaim is an I/O/filesystem interaction, not a free background service — high confidence.** GFP masks and NOFS/NOIO scope exist because reclaim can recurse into subsystems and deadlock if a driver or filesystem holds the wrong resource.

### Lesson connection

Lesson 06 teaches the API-level decision. Follow it with the pinned v7.4-rc4 source in mm/slub.c, mm/page_alloc.c and mm/vmalloc.c; then compare the later 7.4-rc5 merge diffs above. Keep the source baseline and development merge status separate.

### Next checkpoint

- Recheck kernel.org for a new mainline release and linux-next daily tag.
- Re-query Linus's mainline merge history; note mirror divergence and use exact merge hashes when available.
- Do not repeat the memory merges above as new unless a subsequent commit changes the behavior.
- Continue with Lesson 07 — Kernel Object Lifetime, References and Error Unwinding.


## Checkpoint 2026-11-28 09:10 IST — new io_uring/hwmon merges; checkpoint reconciliation

### Release and integration status

- The [kernel.org release index](https://www.kernel.org/) still lists published mainline **7.4-rc4** (2026-11-22), stable **7.3.6** (2026-11-23), and LTS **6.18.65**, **6.12.119**, **6.6.163**, **6.1.193**, **5.15.226** and **5.10.275**.
- The official index still lists **linux-next next-20261127**. No next-20261128 snapshot is visible at this checkpoint.
- The latest visible Linus mainline commit in the GitHub commits history is the [io_uring pull 7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046](https://github.com/torvalds/linux/commit/7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046), committed 2026-11-28 02:56:58 UTC (08:26:58 IST). It is a merged mainline development commit; the public release index still says 7.4-rc4, so do not call this a released 7.4-rc5 feature.

### Upstream briefing

**New since the prior 08:12 IST checkpoint:**

1. **Merged mainline — io_uring fixes, 7.4 development:** [7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046](https://github.com/torvalds/linux/commit/7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046), committed 08:26:58 IST. Includes [a905ff43362e80590e54f6ea9ddb2d1a998aa01c](https://github.com/torvalds/linux/commit/a905ff43362e80590e54f6ea9ddb2d1a998aa01c), which moves the full io_ring_ctx_free() teardown to RCU work so fields are not dismantled while task-work readers can still inspect them; and [6f3159575d53eed2d54557aef2dd8373da7c0208](https://github.com/torvalds/linux/commit/6f3159575d53eed2d54557aef2dd8373da7c0208), which drains cached task_struct references on ring release. Hardware/driver relevance: asynchronous queue teardown, cached references and callbacks are analogous to firmware/DSP/PCIe queue lifetime. This is merged mainline development, not yet a released stable feature.
2. **Merged mainline — hwmon fixes, 7.4 development:** [ad5b48d7188721ba3a7a351d06a78f8030148e61](https://github.com/torvalds/linux/commit/ad5b48d7188721ba3a7a351d06a78f8030148e61), committed 08:23:49 IST. Includes a probe check corrected for single-collection Aquaero interfaces, half-precision PMBus peak comparisons, Lenovo dual-fan ACPI paths and a missing platform module alias. Driver lesson: validate firmware/interface cardinality before indexing optional collections, and ensure bus aliases exist for module autoload.

**Checkpoint reconciliation — merged before 08:12 IST but absent from the prior checkpoint's summary (not new after that checkpoint):**

3. **Merged mainline — workqueue drain fix:** [94ff2d27fafad99c63c02ab4d7e148c37e4eff80](https://github.com/torvalds/linux/commit/94ff2d27fafad99c63c02ab4d7e148c37e4eff80), included in the [workqueue pull](https://github.com/torvalds/linux/commit/e1faf0ed38d4f9b4ca75149428c78cf954586450). A drain pass could miss a chained work item queued onto another pwq and return before the queue was truly empty. The fix tracks started work and rechecks pwqs. The [maintainer report](https://lkml.iu.edu/hypermail/linux/kernel/2611.3/04049.html) includes reproducers and the destroy/leak failure.
4. **Merged mainline — tracing fixes:** [673af170ceca345ce010aeeb1818172844f6b8c9](https://github.com/torvalds/linux/commit/673af170ceca345ce010aeeb1818172844f6b8c9). Includes atomic bitops/READ_ONCE for a trace-event flags race and fixes for ftrace recursion records and user-event fork failure.
5. **Merged mainline — BPF security/correctness fixes:** [d3a5ea8a82fabeb2314700c2e001130faf99df1f](https://github.com/torvalds/linux/commit/d3a5ea8a82fabeb2314700c2e001130faf99df1f). The pull summary describes verifier type/attach checks, CAP_PERFMON requirements where CAP_BPF alone exposed kernel addresses or task operations, speculation barriers, NULL checks and clearing uninitialized data in skb/XDP/socket helpers.
6. **Merged mainline — DRM fixes:** [c639ac6b9cf100b860408a3cf8d7eab268866cd7](https://github.com/torvalds/linux/commit/c639ac6b9cf100b860408a3cf8d7eab268866cd7). Includes dma-fence error-check ordering, TTM LRU cleanup on shutdown, unregistering hwmon before device unplug, GPU error-path/fence accounting and a V3D double-free fix.

The three reconciliation merges above have timestamps earlier than the previous 08:12 IST checkpoint. They are documented now because the earlier search missed them; they are not represented as new post-checkpoint activity.

### Trend observations

- **Object lifetime and teardown — high confidence, recurring:** io_uring now provides fresh evidence for full teardown after an RCU grace period and explicit draining of cached task references; the workqueue fix demonstrates that chained callbacks can outlive an apparently empty drain pass. DRM's device-unplug and double-free fixes reinforce the same requirement.
- **RCU versus reference counting — high confidence:** RCU protects readers traversing a published pointer; a kref protects a successfully acquired object beyond the read-side section. Deferring only the outer free is insufficient if members are torn down while readers still access them.
- **BPF verifier/security hardening — confirmed merged activity:** attach-type, pointer, capability and uninitialized-data checks continue to evolve. Treat verifier acceptance as a safety boundary that itself needs regression tests.
- **Driver probe and firmware-description validation — recurring:** the hwmon collection-count fix and module alias correction show that firmware/interface topology and module autoload are part of correct probe behavior, not incidental packaging.

### Validation and limits

- The latest mainline list was checked through the GitHub commits page and GitHub commit search; exact mainline tip was not fetched from git.kernel.org in this run.
- The official kernel.org index provides the current published release and linux-next snapshot. Direct linux-next Git object/hash was not independently queried; no newer snapshot than next-20261127 was visible.
- No proposed patch or linux-next-only change is presented as merged/released functionality here.
- The sequential curriculum continues with Lesson 08 — Execution Contexts and Synchronization. The new io_uring/workqueue lifetime fixes are used as source-archaeology examples, not as a reason to skip the foundations.

### Next checkpoint

Compare against mainline merge 7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046 and linux-next next-20261127. Recheck the official release index, mainline commit history, subsystem pulls and mailing-list reports.
