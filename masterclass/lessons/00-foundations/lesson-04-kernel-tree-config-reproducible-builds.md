# Lesson 04 — Linux Kernel Source Tree, Configuration and Reproducible Builds

**Stage:** 1 — Kernel architecture and environment  
**Estimated time:** 90 minutes  
**Source baseline:** Linux **v7.4-rc4**  
**Prerequisites:** Lessons 01–03

## 1. Learning objectives

By the end of this lesson, you should be able to:

1. Navigate the kernel source tree and identify the subsystem responsible for a driver or framework.
2. Use MAINTAINERS, get_maintainer.pl, git log and git blame to find ownership and history.
3. Explain the roles of Kconfig, Kbuild, architecture Makefiles and generated headers.
4. Create a configuration from an architecture defconfig and merge a reproducible config fragment.
5. Explain ARCH, CROSS_COMPILE, LLVM, O= and the distinction between source and output trees.
6. Build the right kernel artifacts for x86-64 and arm64 without confusing vmlinux with a boot image.
7. Record enough provenance to reproduce a kernel build and debug a configuration/build failure.

## 2. Why this machinery exists

Linux supports many CPU architectures, boards, buses, device classes and deployment profiles. A single source tree must produce:

- a small embedded kernel with a fixed set of built-in drivers;
- a general-purpose distribution kernel with a large module set;
- a debug kernel with symbols and diagnostics;
- an architecture-specific image that firmware or a bootloader can load;
- external modules built against one exact configured kernel tree.

These requirements cannot be handled safely by a single hand-written Makefile. Kconfig models feature availability and dependencies. Kbuild translates the selected configuration into object selection and build rules. Architecture-specific Makefiles and linker scripts produce the appropriate image and layout.

The separation between source and output trees is also important. It allows one source revision to be built for multiple architectures/configurations without repeatedly destroying generated files or accidentally mixing host and target objects.

**Engineering interpretation:** a kernel build is a configured software product, not just a compiler invocation. The source commit, configuration, architecture, toolchain, generated headers and build commands together define the artifact.

## 3. Source tree map

Start at the repository root. The exact tree grows and changes, but these areas are useful landmarks:

| Path | Typical responsibility |
|---|---|
| arch/ | Architecture entry, CPU-specific code, architecture Kconfig/Makefiles and linker scripts |
| block/ | Block layer and block I/O infrastructure |
| crypto/ | Crypto API and algorithms |
| Documentation/ | Kernel and subsystem documentation |
| drivers/ | Hardware drivers and driver frameworks |
| fs/ | VFS and filesystem implementations |
| include/ | Public/internal headers and architecture headers |
| init/ | Early initialization and initcall orchestration |
| ipc/ | System V IPC and related infrastructure |
| kernel/ | Core scheduler, locking, time, workqueues and other core facilities |
| lib/ | Generic library helpers |
| mm/ | Memory management |
| net/ | Networking stack and protocols |
| rust/ | Rust support and abstractions |
| scripts/ | Kconfig, Kbuild, modpost, helper and validation scripts |
| security/ | LSM and security infrastructure |
| sound/ | ALSA, ASoC and sound drivers |
| tools/ | Userspace tooling such as perf and selftests support |
| virt/ | Virtualization support |

A driver may span more than one directory. For example, a PCI audio device can involve a bus driver under drivers/, generic DMA APIs in kernel/ and include/, ALSA code under sound/, and a Device Tree or ACPI description. Follow the call path and subsystem boundaries rather than assuming one directory owns everything.

### 3.1 Find the responsible subsystem

The MAINTAINERS file contains patterns and ownership metadata. Common fields include:

- M: maintainer;
- R: reviewer;
- L: mailing list;
- S: maintenance status;
- F: file or path pattern;
- X: excluded path pattern;
- T: source control tree.

From the kernel source root:

~~~bash
scripts/get_maintainer.pl drivers/soundwire/bus.c
scripts/get_maintainer.pl drivers/pci/controller/pcie-altera.c
scripts/get_maintainer.pl sound/soc/soc-core.c
~~~

For a patch, pass the patch file instead. Review the output; it is a routing aid, not a guarantee that every listed person must receive every message.

Useful history commands:

~~~bash
git log --oneline -- drivers/soundwire/
git log --follow -- path/to/driver.c
git blame -L 100,180 -- path/to/driver.c
git log -- Documentation/kbuild/
~~~

Before changing a driver, inspect the subsystem's recent accepted patches, documentation, bindings and tests. A technically correct patch sent to the wrong subsystem list or against the wrong tree will lose time.

## 4. Kconfig: selecting the product

Kconfig defines configuration symbols, their types, dependencies, choices and help text.

### 4.1 Symbol types

- bool: enabled (y) or disabled (n).
- tristate: built-in (y), module (m) or disabled (n), subject to module support and dependencies.
- int, hex and string: values rather than y/m/n.
- choice: selects one option from a set, subject to the choice's rules.

A tristate driver can be built into the kernel or as a module. The selected value is not the same thing as the runtime state of a device.

### 4.2 Dependency semantics

- depends on restricts whether an option can be enabled and what values are valid.
- select forces another symbol and can bypass that symbol's own dependency checks; it must be used carefully.
- imply is weaker than select and may be overridden by user choice or dependencies.
- default supplies a value when the user has not selected one.
- choice groups mutually exclusive alternatives.

Do not assume that setting a symbol to y in a fragment guarantees that it remains y. Its dependencies and enclosing choice may force a different result. Always inspect the final .config after olddefconfig.

### 4.3 Defconfig and configuration targets

A defconfig is a maintained baseline for an architecture or platform. Examples:

~~~bash
make O=$OUT ARCH=x86 x86_64_defconfig
make O=$OUT ARCH=arm64 defconfig
~~~

Common targets:

| Target | Purpose |
|---|---|
| defconfig / architecture defconfig | Start from a maintained baseline |
| olddefconfig | Resolve new/changed symbols using defaults without prompting |
| oldconfig | Ask about new symbols interactively |
| menuconfig | Interactive ncurses configuration |
| nconfig | Alternative interactive interface |
| savedefconfig | Write a minimal defconfig representing the current configuration |
| allnoconfig | Start with almost everything disabled |
| allyesconfig | Enable as much as possible, subject to dependencies |
| allmodconfig | Select modules where possible |
| randconfig | Generate a randomized configuration |
| tinyconfig | Small configuration, where supported |

Large all* configurations can be expensive or impossible for some architectures/toolchains. They are useful for coverage and compile testing, not automatically good production configurations.

### 4.4 Configuration fragments

A fragment is a small text file containing desired symbol values. Keep it under version control and merge it into a known architecture baseline.

Example fragment from this lesson:

~~~text
CONFIG_MODULES=y
CONFIG_MODULE_UNLOAD=y
CONFIG_DEBUG_KERNEL=y
CONFIG_DEBUG_INFO=y
CONFIG_KALLSYMS=y
CONFIG_KALLSYMS_ALL=y
CONFIG_IKCONFIG=y
CONFIG_IKCONFIG_PROC=y
~~~

Example workflow:

~~~bash
SRC=$HOME/src/linux
OUT=$HOME/build/linux-x86-debug
FRAG=$PWD/debug.fragment

make -C "$SRC" O="$OUT" ARCH=x86 x86_64_defconfig
cp "$OUT/.config" "$OUT/base.config"
"$SRC/scripts/kconfig/merge_config.sh" -m -O "$OUT" "$OUT/.config" "$FRAG"
make -C "$SRC" O="$OUT" ARCH=x86 olddefconfig
"$SRC/scripts/diffconfig" "$OUT/base.config" "$OUT/.config"
~~~

The exact merge command and configuration targets are documented in the pinned Kconfig documentation. Fragments are applied in order; later fragments can override earlier settings. The merge script can report redefinitions. The final olddefconfig pass resolves dependencies and defaults.

**Important:** the sample fragment is a learning/debug profile, not a universal production configuration. Some options may be unavailable or constrained on another architecture or kernel revision. Verify the final .config.

### 4.5 Inspect and edit configuration safely

The scripts/config helper can inspect or change symbols in a specific config file:

~~~bash
"$SRC/scripts/config" --file "$OUT/.config" --state MODULES
"$SRC/scripts/config" --file "$OUT/.config" --enable KALLSYMS
make -C "$SRC" O="$OUT" ARCH=x86 olddefconfig
~~~

For scripted configuration changes, always run olddefconfig afterward and inspect the result. Do not edit generated headers under include/generated by hand.

## 5. Kbuild: turning configuration into objects

Kconfig chooses the feature set; Kbuild decides which objects are compiled and how they are linked.

A typical in-tree driver directory may contain:

~~~make
obj-$(CONFIG_EXAMPLE_DRIVER) += example.o
example-y := core.o hw.o dma.o
example-$(CONFIG_DEBUG_FS) += debug.o
~~~

The CONFIG value determines whether the composite object is built in, as a module, or omitted. The composite list determines which object files are linked into example.o.

The top-level Makefile establishes the build environment. Architecture Makefiles supply target-specific settings. Directory Kbuild files select objects. Common scripts compile them, modpost checks module symbols/metadata, and the final link produces vmlinux. Architecture-specific targets package the boot image.

Useful pinned files:

- top-level Makefile;
- arch/x86/Makefile and arch/arm64/Makefile;
- scripts/Makefile.build;
- scripts/Makefile.modpost;
- scripts/link-vmlinux.sh;
- scripts/kconfig/Makefile.

The build graph is not a substitute for runtime dependency management. A driver must still handle provider readiness, deferred probe, firmware availability and device power sequencing.

## 6. Source tree versus output tree

Use an out-of-tree build:

~~~bash
SRC=$HOME/src/linux
OUT=$HOME/build/linux-x86-debug

make -C "$SRC" O="$OUT" ARCH=x86 x86_64_defconfig
make -C "$SRC" O="$OUT" ARCH=x86 olddefconfig
make -C "$SRC" O="$OUT" ARCH=x86 -j8 vmlinux modules
~~~

- SRC is the source checkout.
- OUT holds .config, generated headers, object files, vmlinux and modules.
- O= selects the output directory.
- KBUILD_OUTPUT can also define an output tree, but explicit O= is easier to audit in scripts.

Do not share one output directory between different architectures or incompatible configurations. Use distinct output directories, for example linux-x86-debug and linux-arm64-release.

## 7. Architecture and toolchain selection

### 7.1 ARCH is the kernel architecture

Common values include:

| Target | ARCH |
|---|---|
| x86-64 | x86 |
| arm64 | arm64 |
| 32-bit ARM | arm |
| RISC-V | riscv |

Do not pass uname -m blindly as ARCH. The userspace machine name and Kbuild architecture name are not always identical.

### 7.2 CROSS_COMPILE selects a GNU cross-tool prefix

For a GNU arm64 toolchain:

~~~bash
make -C "$SRC" O="$OUT" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make -C "$SRC" O="$OUT" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image modules dtbs
~~~

The prefix is used to locate target compiler/binutils tools. Verify that the compiler, assembler, linker and utilities are mutually compatible.

### 7.3 LLVM builds

Kbuild supports LLVM toolchains:

~~~bash
make -C "$SRC" O="$OUT" ARCH=arm64 LLVM=1 defconfig
make -C "$SRC" O="$OUT" ARCH=arm64 LLVM=1 -j8 Image modules dtbs
~~~

A versioned LLVM selection can be used where supported by the toolchain and Kbuild documentation. Record the exact clang/LLD version. Do not silently mix host GCC-built objects with target Clang-built objects in one output directory.

## 8. Choose the correct build artifacts

| Artifact/target | Meaning |
|---|---|
| vmlinux | Final linked kernel ELF, useful for symbols and debugging |
| modules | Build loadable modules selected by the configuration |
| bzImage | x86 bootable compressed image target |
| Image | arm64 kernel image target |
| dtbs | Build Device Tree blobs selected by the architecture/configuration |
| modules_install | Install modules into a staging root or target filesystem |
| headers_install | Install userspace kernel headers into a staging path |
| kernelrelease | Print the configured kernel release string |

Example x86-64 build:

~~~bash
make -C "$SRC" O="$OUT" ARCH=x86 -j8 vmlinux bzImage modules
~~~

Example arm64 build:

~~~bash
make -C "$SRC" O="$OUT" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 vmlinux Image modules dtbs
~~~

The actual image filename and available targets are architecture-dependent. Use make help and the architecture documentation rather than assuming every target has the same name.

Stage modules without modifying the host:

~~~bash
ROOTFS=$HOME/stage/rootfs
make -C "$SRC" O="$OUT" ARCH=x86 INSTALL_MOD_PATH="$ROOTFS" modules_install
~~~

Do not run modules_install or make install against a live host unless that is explicitly the intended deployment and you have a tested recovery path. For learning, stage into a disposable root filesystem or VM image.

## 9. Reproducibility: what must be recorded

A build is not reproducible merely because the command is written down. Record:

- exact source commit and whether the worktree is dirty;
- kernel version/tag and any local patch series;
- architecture and target CPU/ABI;
- complete .config and any fragments, in application order;
- compiler, assembler, linker and binutils versions;
- relevant environment variables (ARCH, CROSS_COMPILE, LLVM, CC, LD, HOSTCC);
- build command, parallelism and warning options;
- generated artifacts and hashes;
- host OS/container/VM details where they affect the build.

Useful commands:

~~~bash
git -C "$SRC" rev-parse HEAD
git -C "$SRC" status --short
git -C "$SRC" describe --tags --always --dirty
make -s -C "$SRC" O="$OUT" ARCH=x86 kernelversion
make -s -C "$SRC" O="$OUT" ARCH=x86 kernelrelease
sha256sum "$OUT/.config" "$OUT/vmlinux"
gcc --version
clang --version
ld --version
~~~

For deterministic comparisons, preserve the source revision, configuration, toolchain and build environment. Build timestamps, user/host metadata, signing keys and other inputs can affect byte-for-byte identity. Consult the kernel reproducible-build documentation before claiming two builds are identical.

## 10. Lab — audit a kernel source/output tree

The lab directory contains:

- kernel-tree-audit.sh — a read-only source/output-tree audit helper;
- debug.fragment — an example configuration fragment.

Run:

~~~bash
chmod +x kernel-tree-audit.sh
./kernel-tree-audit.sh "$HOME/src/linux" "$HOME/build/linux-x86-debug" x86
~~~

The script reports the source revision/dirty state when Git metadata exists, kernel version, output-tree config presence, kernel release, selected config symbols, generated-header presence and compiler environment. It does not build, install, boot or load a module.

Then perform the real configuration steps:

~~~bash
SRC=$HOME/src/linux
OUT=$HOME/build/linux-x86-debug

make -C "$SRC" O="$OUT" ARCH=x86 x86_64_defconfig
cp "$OUT/.config" "$OUT/base.config"
"$SRC/scripts/kconfig/merge_config.sh" -m -O "$OUT" "$OUT/.config" "$PWD/debug.fragment"
make -C "$SRC" O="$OUT" ARCH=x86 olddefconfig

grep -E '^(CONFIG_(MODULES|DEBUG_INFO|KALLSYMS|KALLSYMS_ALL|IKCONFIG|IKCONFIG_PROC)=|# CONFIG_.* is not set)' "$OUT/.config"
"$SRC/scripts/diffconfig" "$SRC/arch/x86/configs/x86_64_defconfig" "$OUT/.config"
~~~

If you have a compatible build host and sufficient disk/RAM, build in the output directory:

~~~bash
make -C "$SRC" O="$OUT" ARCH=x86 -j8 W=1 vmlinux bzImage modules
~~~

Expected results on a successful full build include vmlinux, the x86 boot image under the architecture output path, selected .ko files and generated headers. Exact artifacts depend on configuration and kernel revision.

### Lab validation status for this publication

- bash -n passed for kernel-tree-audit.sh.
- The script's control flow was executed against a deliberately minimal mock source/output fixture; it correctly reported a mock kernel version, config symbols and missing generated headers.
- This mock test validates script behavior only. It is not a Linux source-tree audit.
- The authoring environment has kernel headers but not a full Linux source checkout. A real v7.4-rc4 Kconfig merge, full kernel build, arm64 cross-build and QEMU boot were not performed.

## 11. Debugging scenarios

### Scenario A — A requested CONFIG symbol is missing from .config

Check the symbol spelling, whether it exists in this kernel revision, its type, depends on chain, enclosing choice, architecture constraints and whether a later fragment overrides it. Run olddefconfig, inspect merge_config output and search the Kconfig definition. Do not force the symbol by editing generated headers.

### Scenario B — The kernel builds on x86 but the arm64 build fails immediately

Check ARCH, CROSS_COMPILE or LLVM target, compiler/binutils versions, host tool dependencies and whether the output directory contains stale x86 objects. Use a fresh arm64 output tree. Verify the cross compiler can compile a trivial arm64 target before blaming kernel source.

### Scenario C — A module exists but is not installed into the staging root

Check whether the driver was configured as m rather than y, whether the module target completed, INSTALL_MOD_PATH, depmod behavior and the module install logs. Built-in objects do not appear as .ko files.

### Scenario D — A build unexpectedly uses the host's old configuration

Inspect O=, KBUILD_OUTPUT and KCONFIG_CONFIG. A stale environment variable or reused output directory can redirect generated files. Print the absolute SRC and OUT paths in build scripts.

### Scenario E — A patch is sent to the wrong maintainers

Run scripts/get_maintainer.pl on the patch and inspect MAINTAINERS patterns, subsystem documentation and recent pull-request history. Confirm the target tree and mailing list before sending.

### Scenario F — Two developers cannot reproduce each other's build

Compare source commit, dirty diff, .config, fragment order, architecture, toolchain versions, environment variables and generated artifacts. A pasted command without those inputs is not a reproducible build record.

## 12. Exercises

- **Beginner:** Navigate the source tree and identify the directories/files involved in a PCI audio driver, a SoundWire controller and an IOMMU provider.
- **Beginner:** Run get_maintainer.pl for one driver file. Explain the difference between M, R, L, F and T entries in MAINTAINERS.
- **Intermediate:** Create an x86 defconfig, merge debug.fragment, run olddefconfig and explain any requested symbols that did not remain enabled.
- **Intermediate:** Build the same source revision in two different O= directories with different configuration fragments. Compare the resulting .config files with scripts/diffconfig.
- **Advanced:** Prepare an arm64 cross-build using a fresh output tree. Record the compiler triple, LLVM/GNU tool versions, ARCH, CROSS_COMPILE, source commit and image target. Do not claim success until the build actually completes.
- **Advanced:** Pick a driver patch from a subsystem tree. Use get_maintainer.pl, MAINTAINERS, git log and the subsystem documentation to reconstruct the correct review route and base tree.
- **Staff-level design:** Design a CI matrix for one SoC driver across x86-64 mock builds, arm64 defconfig, allmodconfig coverage, Sparse, KUnit and hardware validation. Explain which checks are gating, which are advisory and how you control matrix cost.

## 13. Knowledge check and answer checkpoints

1. **What is the difference between Kconfig and Kbuild?**  
   Kconfig describes selectable features and constraints. Kbuild translates the effective configuration and Makefiles into compilation/link rules and selected objects.

2. **Why is ARCH=x86 used for x86-64 rather than ARCH=x86_64?**  
   Kbuild's architecture directory and configuration use x86 as the architecture identifier; userspace uname output is not necessarily the Kbuild ARCH value.

3. **What does O= isolate?**  
   Generated configuration, headers, objects, linked images and modules in a separate output tree.

4. **Why might a fragment's CONFIG_FOO=y not survive olddefconfig?**  
   The symbol may not exist, may be constrained by dependencies/choice, may be overridden by a later fragment, or may be unavailable on the selected architecture.

5. **What is the risk of select in Kconfig?**  
   It can force a symbol without satisfying that symbol's normal dependencies, creating invalid or surprising configurations.

6. **What does get_maintainer.pl provide?**  
   Candidate maintainers, reviewers, mailing lists and relevant routing metadata based on MAINTAINERS and repository context; the contributor must still verify subsystem guidance.

7. **Why keep x86 and arm64 builds in separate output directories?**  
   Generated headers, configuration and objects are architecture-specific. Reusing an output tree can mix incompatible build artifacts.

8. **What is the difference between vmlinux and bzImage/Image?**  
   vmlinux is the linked kernel ELF; bzImage and arm64 Image are architecture-specific boot image outputs.

9. **Why stage modules with INSTALL_MOD_PATH?**  
   It installs into a target root without modifying the running host's module directory.

10. **What evidence is required before calling a build reproducible?**  
    Source revision/dirty state, configuration and fragment order, architecture, toolchain, environment and build command must be recorded; byte identity also requires controlling timestamps, signing and other nondeterministic inputs.

## 14. Pinned source and documentation references

All source links below are pinned to Linux v7.4-rc4:

- [Kernel build system overview](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kbuild.rst)
- [Kbuild Makefiles](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/makefiles.rst)
- [Kconfig language](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kconfig-language.rst)
- [Configuration targets and Kconfig](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kconfig.rst)
- [External modules](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/modules.rst)
- [Reproducible builds](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/reproducible-builds.rst)
- [LLVM toolchain support](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/llvm.rst)
- [MAINTAINERS](https://github.com/torvalds/linux/blob/v7.4-rc4/MAINTAINERS)
- [get_maintainer.pl](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/get_maintainer.pl)
- [Kconfig merge helper](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/kconfig/merge_config.sh)
- [scripts/config](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/config)
- [Kconfig top-level file](https://github.com/torvalds/linux/blob/v7.4-rc4/init/Kconfig)
- [x86 Kconfig](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/Kconfig)
- [arm64 Kconfig](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/Kconfig)
- [x86-64 defconfig](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/configs/x86_64_defconfig)
- [arm64 defconfig](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/configs/defconfig)
- [Top-level Makefile](https://github.com/torvalds/linux/blob/v7.4-rc4/Makefile)
- [Kbuild object compilation](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/Makefile.build)
- [Module post-processing](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/Makefile.modpost)
- [Final vmlinux link](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/link-vmlinux.sh)

## 15. Completion criteria

This lesson is complete when you can inspect a real source tree, identify the subsystem/maintainers, create a configuration from a known baseline, merge and verify a fragment, build into a clean output directory and record all build inputs. The included audit script has only been validated against a mock fixture in this publication; a real full-kernel build remains a separate lab task.

**Next lesson:** Lesson 05 — Kernel C runtime essentials, modules, logging and safe debugging. This starts the first kernel-code execution lessons after source/configuration/build foundations.
