# Lesson 03 — C, ELF, Linker and Kernel Build Foundations

**Stage:** 0 and 1  
**Suggested time:** 90 minutes  
**Source baseline:** Linux v7.4-rc4  
**Prerequisites:** [Lesson 01 — User/kernel boundary](lesson-01-user-kernel-boundary.md) and [Lesson 02 — Linux boot and early initialization](lesson-02-linux-boot-and-early-init.md)

## 1. Learning objectives

By the end of this lesson, you should be able to:

- Read kernel-oriented C declarations, pointers, structures, integer types and bit masks without confusing CPU virtual, physical and DMA addresses.
- Explain why kernel code uses GNU C extensions and compiler/static-analysis annotations.
- Predict structure padding and alignment, and recognize when packed data is unsafe to access directly.
- Read ELF headers, sections, symbols and relocations using readelf, objdump and nm.
- Explain how a C translation unit becomes an object file, a module or part of the linked kernel.
- Read the purpose of Kconfig and Kbuild statements such as obj-y, obj-m and obj-$(CONFIG_FOO).
- Build and inspect a userspace ELF program and compile a small external kernel module without loading it.
- Diagnose common build, symbol, ABI and module-version failures.

## 2. Why this matters to a driver engineer

A hardware driver is not just C code that writes registers. It is compiled under architecture-specific rules, placed into ELF sections, linked with other objects, described by configuration, and initialized in an order that must agree with the hardware dependencies.

A few examples make this concrete:

- A DMA descriptor has a hardware-defined byte layout. A compiler-added padding byte, incorrect endianness or wrong alignment can make the device fetch the wrong address.
- A register field is a bit-level contract. A C bitfield does not define a portable hardware register layout.
- A function marked __init may be discarded after initialization. Retaining its address in a long-lived callback table is a lifetime bug.
- A driver selected as a module cannot help mount the root filesystem unless the module and its dependencies are present in the initramfs.
- Built-in object/link order can affect initcall order. An implicit dependency between providers and consumers can therefore become a boot-time race.
- A module that compiles against one kernel's headers is not automatically loadable into a different running kernel.

The goal is to understand the complete path:

~~~text
C source
  -> preprocessor
  -> compiler / assembler
  -> ELF relocatable object (.o)
  -> Kbuild archive and link steps
  -> vmlinux or module (.ko)
  -> architecture-specific boot image / module loader
  -> runtime initialization
~~~

## 3. Kernel C: the language and the contracts

### 3.1 GNU C, not arbitrary application C

The kernel uses a controlled GNU C dialect and a carefully selected compiler/toolchain configuration. Kbuild supplies architecture-specific flags, include paths, generated headers, instrumentation and hardening options.

Do not copy a userspace command such as 'gcc driver.c -o driver' and assume it represents a kernel build. Kernel compilation needs:

- the configured source/output tree;
- generated headers and configuration;
- the target architecture and ABI;
- kernel-specific compiler flags;
- the correct symbol and module metadata;
- the kernel's linker and post-link steps.

Use the [kernel programming-language documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/process/programming-language.rst) and [coding-style guide](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/process/coding-style.rst) as the authority for the selected revision.

### 3.2 Kernel integer and address types

Common types include:

| Type | Meaning | Driver caution |
|---|---|---|
| u8, u16, u32, u64 | Unsigned fixed-width CPU integers | Width is explicit; byte order is not |
| s8, s16, s32, s64 | Signed fixed-width integers | Do not use for an unsigned hardware field |
| __le16, __le32, __le64 | Values stored in little-endian format | Convert at the CPU/device boundary |
| __be16, __be32, __be64 | Values stored in big-endian format | Convert at the CPU/device boundary |
| size_t | Size of an object in the current C ABI | Width follows the target ABI |
| phys_addr_t | CPU physical address type | Not a device DMA address |
| dma_addr_t | Address used by a device for DMA | May be an IOVA and must come from the DMA API |
| resource_size_t | Size/address type for resources | Do not truncate to u32 without a hardware/API reason |

The critical distinction for SoC drivers is:

- a CPU virtual address is what the CPU dereferences;
- a CPU physical address describes system memory;
- a DMA address is what the device is programmed to use.

They may have the same numeric value on a simple platform, but that is not a contract. An IOMMU can translate the device-visible IOVA to a different physical page.

Use the definitions in [include/linux/types.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/types.h). For byte-order conversion, inspect [include/linux/byteorder/generic.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/byteorder/generic.h).

### 3.3 Pointers, arrays and object layout

For an array:

~~~c
u32 values[8];
size_t bytes = sizeof(values);       /* size of all eight elements */
size_t count = ARRAY_SIZE(values);  /* element count */
~~~

After an array is passed to a function, its parameter is normally adjusted to a pointer. Inside that function, sizeof(pointer) is not the size of the caller's array. Pass the element count or byte count explicitly.

Pointer arithmetic is scaled by the pointed-to type. For example, p + 1 advances by sizeof(*p) bytes, not one byte. Use a byte pointer only when the operation is genuinely byte-oriented.

offsetof() and container_of() are fundamental kernel techniques. A subsystem often embeds a framework object inside a driver-private structure and recovers the enclosing object from a member pointer. The operation is only valid when the pointer really refers to that member in a live enclosing object. It does not create ownership or extend lifetime.

### 3.4 Structure padding, alignment and packed data

The compiler may insert padding to satisfy alignment. Consider:

~~~c
struct dma_desc {
        u64 dma_addr;
        u32 length;
        u32 control;
};
~~~

On the tested x86-64 ABI, the lab reports a 16-byte structure with offsets 0, 8 and 12. That result is for that target and declaration; verify layout for every supported architecture and compiler configuration.

A packed wire header can have a member at an unaligned offset:

~~~c
struct packed_header {
        u8 tag;
        u32 value;
} __packed;
~~~

This can be appropriate for a byte-defined wire format, but it is not a general solution for MMIO registers or DMA descriptors. Unaligned loads may be slower or fault on some architectures. Use the subsystem's unaligned access helpers when decoding packed byte streams. For hardware descriptors, use the exact documented layout, explicit endian types/conversions, required alignment and compile-time size/offset checks.

Never take the address of a packed member and pass it to code that assumes normal alignment unless the API explicitly supports unaligned access.

### 3.5 Bit masks are clearer than C bitfields for registers

C bitfield layout, allocation order and packing are implementation-defined. Do not describe a device register with a C bitfield and assume the compiler will match the silicon.

Use masks and shifts:

~~~c
#define DESC_OWN        BIT(0)
#define DESC_IRQ        BIT(1)
#define DESC_LENGTH     GENMASK(15, 4)

control = FIELD_PREP(DESC_LENGTH, length) | DESC_OWN;
length  = FIELD_GET(DESC_LENGTH, control);
~~~

The real kernel helpers provide type/range checks and are preferable to hand-written arithmetic. The lab's small FIELD_PREP analogue exists only to make the bit operation visible; it is not a replacement for [include/linux/bitfield.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/bitfield.h) or [include/linux/bits.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/bits.h).

For MMIO, use the I/O accessors such as readl() and writel() rather than dereferencing an ordinary C pointer. The __iomem annotation helps Sparse identify invalid address-space use; it does not itself perform an MMIO transaction.

### 3.6 Annotations are part of the interface

Common annotations communicate constraints to the compiler, Sparse, the linker or kernel developers:

- __user: pointer belongs to userspace; use the user-access APIs rather than dereferencing it.
- __iomem: pointer refers to I/O memory; use the appropriate I/O accessors.
- __must_check: callers must inspect the result.
- __init: code/data are placed in init sections and may be reclaimed after initialization.
- __exit: exit-only code, with different treatment for built-in code and modules.
- __packed / __aligned(n): layout/alignment attributes; use only when required by an external contract.
- __rcu: marks RCU-protected pointer use for analysis and review.

These are not decoration. For example, storing a pointer to an __init function in a callback used after boot can create a call into reclaimed memory. An __iomem pointer is not a normal RAM pointer. A __user pointer must not be trusted or directly dereferenced.

See [include/linux/compiler_types.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/compiler_types.h), [include/linux/compiler_attributes.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/compiler_attributes.h) and the [Sparse documentation](https://docs.kernel.org/dev-tools/sparse.html).

## 4. ELF: what the compiler and linker actually produce

ELF is the object-file format used by Linux toolchains. A useful first distinction is:

- .o object files and loadable .ko modules are normally ELF relocatable objects (ET_REL);
- the final linked kernel ELF is vmlinux;
- an architecture may then transform/package that image into a boot format such as x86 bzImage or arm64 Image.

The boot image is not interchangeable with the linked vmlinux.

### 4.1 Important ELF structures

| ELF item | What it tells you |
|---|---|
| ELF header | Class (32/64 bit), endianness, machine and file type |
| Section header table | Sections such as .text, .rodata, .data, .bss, .symtab and .rela.* |
| Symbol table | Functions, objects, section symbols, binding and visibility |
| String tables | Names used by symbol and section tables |
| Relocation sections | Places that need addresses/values resolved by a later link or loader |
| Program headers | Loadable segments in executable/shared-object style files; not the main organizing view of a relocatable .o |

A typical object has:

- .text: machine instructions;
- .rodata: read-only constants;
- .data: initialized writable data;
- .bss: zero-initialized storage;
- .symtab / .strtab: symbols and their names;
- .rela.text or .rel.*: relocation records;
- .debug_*: optional debugging information.

The exact section names and relocation formats depend on architecture and toolchain.

### 4.2 Relocations explain unresolved addresses

When the compiler emits a call to an external function or references a global object, it may not know the final address. It emits a relocation describing what must be fixed later.

Inspect an object:

~~~bash
readelf -h -S -s -r elf_layout_O0.o
objdump -dr elf_layout_O0.o
nm -S elf_layout_O0.o
~~~

In the lab, .rela.text contains relocations for calls to printf and references to constant sections. The disassembly shows the instruction bytes and the relocation entry at the corresponding offset.

This is why an object file can contain a call whose final destination is not yet encoded. The linker or module loader resolves the relocation when the final placement is known.

A .ko remains relocatable because the kernel module loader must place its sections in memory and apply architecture-specific relocations. modpost also checks symbol references and module metadata before the module is finalized.

### 4.3 Why the linker script matters to the kernel

A linker script is not merely a way to concatenate object files. It defines section placement, alignment, exported boundary symbols and special ranges used by the runtime.

Examples include:

- init and exit sections;
- exception tables;
- per-CPU data;
- alternative instruction regions;
- tracepoint and linker-collected metadata;
- initcall function-pointer ranges.

For x86-64, inspect [arch/x86/kernel/vmlinux.lds.S](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/kernel/vmlinux.lds.S) and [include/asm-generic/vmlinux.lds.h](https://github.com/torvalds/linux/blob/v7.4-rc4/include/asm-generic/vmlinux.lds.h). Architecture linker scripts are preprocessed and use common linker-script definitions.

The final link is driven by [scripts/link-vmlinux.sh](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/link-vmlinux.sh), with Kbuild rules and architecture-specific post-processing around it.

### 4.4 Initcall order is connected to build/link order

Initcall macros place function pointers into ordered linker sections. The linker collects those sections and the kernel walks them during initialization. Within a given level, link order can matter.

This means a Kbuild Makefile is not always a cosmetic list. If two providers/consumers have an implicit dependency, relying on accidental object order is fragile. Prefer an explicit dependency or an appropriate initcall level when the subsystem design calls for it.

We will revisit initcalls in the boot and driver-model lessons. For now, remember that compilation and linking can influence runtime initialization order.

## 5. Kconfig and Kbuild: how the kernel decides what exists

### 5.1 Kconfig describes selectable features

Kconfig defines options and their dependencies. A bool is built-in or disabled. A tristate can be built-in (y), module (m) or disabled (n), subject to dependencies and the global module configuration.

Example in-tree fragment:

~~~text
config MASTERCLASS_HELLO
        tristate "Masterclass hello module"
        help
          Small example used to study Kbuild and module metadata.
~~~

Kconfig is not a general-purpose programming language. depends on restricts visibility/valid values. select forces another symbol and should be used carefully because it can bypass that symbol's dependencies. imply is weaker. Read [Documentation/kbuild/kconfig-language.rst](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kconfig-language.rst).

### 5.2 Kbuild connects configuration to object files

The most common forms are:

~~~make
obj-y += core.o
obj-m += example.o
obj-$(CONFIG_MASTERCLASS_HELLO) += lesson03_hello.o
~~~

Interpretation:

- obj-y: compile and include the object in the built-in kernel.
- obj-m: build the object as a loadable module.
- obj-$(CONFIG_FOO): select built-in/module/disabled behavior based on the configured value of CONFIG_FOO.

A composite module can be expressed as:

~~~make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
my_driver-y := core.o hw.o dma.o
my_driver-$(CONFIG_DEBUG_FS) += debug.o
~~~

The my_driver-y list contributes objects to the composite module. The exact Kbuild variable family and scope matter; a normal userspace Makefile is not a substitute.

### 5.3 The build tree has several layers

The high-level model is:

1. Top-level Makefile reads configuration and sets up the build environment.
2. arch/$(SRCARCH)/Makefile supplies architecture-specific settings.
3. Subdirectory Kbuild files select objects based on configuration.
4. Common scripts/Makefile.* rules compile and assemble objects.
5. Archives and link steps construct the intermediate kernel objects and final vmlinux.
6. Architecture-specific targets create the bootable image.
7. Module post-processing resolves/checks symbols and emits .ko files.

Read the [Kbuild Makefiles documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/makefiles.rst), [scripts/Makefile.build](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/Makefile.build), [scripts/Makefile.modpost](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/Makefile.modpost) and the top-level [Makefile](https://github.com/torvalds/linux/blob/v7.4-rc4/Makefile).

### 5.4 Useful artifacts

| Artifact | Why you inspect it |
|---|---|
| .config | Effective kernel feature selection |
| include/generated/ | Generated configuration and architecture headers |
| .o | Compiled relocatable object |
| built-in.a / vmlinux.a | Intermediate archives used by the configured build |
| vmlinux.o | Intermediate linked kernel object in the current build flow |
| vmlinux | Final linked kernel ELF |
| System.map | Kernel symbol/address map for diagnostics |
| Module.symvers | Exported symbol/version information used for module builds |
| modules.order | Module ordering metadata |
| .ko | Loadable kernel module ELF |

The exact intermediate artifacts vary by release, architecture, configuration and link mode. Do not write scripts that assume every architecture produces the same boot-image filename.

### 5.5 Reproducible out-of-tree builds

Use a separate output directory so generated files do not pollute the source tree:

~~~bash
make O=$HOME/build/linux-x86_64 x86_64_defconfig
make O=$HOME/build/linux-x86_64 olddefconfig
make O=$HOME/build/linux-x86_64 -j"$(nproc)"
make O=$HOME/build/linux-x86_64 modules
~~~

For an arm64 cross build, the shape is:

~~~bash
make O=$HOME/build/linux-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make O=$HOME/build/linux-arm64 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)"
~~~

With LLVM toolchains, use the supported LLVM=1 or a specific LLVM prefix/version consistently for the build. Do not mix host and target objects.

For an external module:

~~~bash
make -C /path/to/configured/kernel/build M="$PWD" modules
~~~

/path/to/configured/kernel/build must correspond to the intended target kernel. modules_prepare prepares much of the tree for external modules, but it does not generate a complete Module.symvers for CONFIG_MODVERSIONS; a full kernel build is needed for that symbol-version information.

Do not load a module just because compilation succeeded. Verify its architecture, vermagic, symbol dependencies, signing requirements and target kernel first.

## 6. Hands-on lab A — inspect C layout and ELF

The lab is in [lessons/00-foundations/lesson-03-c-elf-kernel-build/](lesson-03-c-elf-kernel-build/).

### Build and run

~~~bash
cd lesson-03-c-elf-kernel-build
make clean all
./elf_layout_O0
./elf_layout_O2
~~~

Expected output on the tested x86-64 ABI:

~~~text
pointer_size=8
dma_desc_size=16 addr_off=0 length_off=8 control_off=12
packed_header_size=5 value_off=1
desc_addr=0x12345000 length=4096 control=0x00000405
wire_tag=0xa5 wire_value=0x11223344
marker=ELF_SECTION_MARKER
~~~

Inspect the relocatable object:

~~~bash
readelf -h -S -s -r elf_layout_O0.o
objdump -dr elf_layout_O0.o
nm -S elf_layout_O0.o
~~~

Find .rodata.lesson, .rela.text, the lesson_marker symbol and relocations for printf. Compare the -O0 and -O2 disassembly. The program's output should remain the same, but the compiler may inline, eliminate or transform code.

The lab intentionally includes a packed header and a custom ELF section. It is a userspace ABI/layout demonstration, not a real DMA descriptor implementation.

## 7. Hands-on lab B — build a kernel module with Kbuild

The kbuild/ subdirectory contains:

- lesson03_hello.c: minimal module init/exit callbacks;
- Makefile: external module Kbuild declaration;
- Kconfig: illustrative in-tree configuration entry.

Build only:

~~~bash
KDIR=/path/to/configured/kernel/build
make -C "$KDIR" M="$PWD/kbuild" modules
file kbuild/lesson03_hello.ko
readelf -h kbuild/lesson03_hello.ko
modinfo kbuild/lesson03_hello.ko
~~~

Expected artifact: lesson03_hello.ko, an ELF relocatable object with module metadata. Do not run insmod on a production machine. Use a disposable VM and an exactly matching kernel build for a later load/unload exercise.

The Kconfig file illustrates an in-tree option. The external module Makefile uses obj-m directly, so this lab does not modify a kernel source tree or its .config.

## 8. Debugging exercises and failure modes

### Scenario A — module compilation succeeds but modpost reports an undefined symbol

Check:

1. Is the symbol actually exported by the target kernel (EXPORT_SYMBOL / EXPORT_SYMBOL_GPL)?
2. Is the required subsystem/configuration enabled?
3. Is the module using the right Module.symvers?
4. Is the symbol in an export namespace requiring an import?
5. Is the module being built against the intended kernel output tree?

A function being non-static in a C file does not make it a kernel module API.

### Scenario B — insmod reports “Invalid module format”

Inspect dmesg, modinfo, vermagic, ELF machine/class, kernel release, SMP/preemption/config differences, symbol versioning and module signature policy. A .ko built against different headers can be a valid ELF file and still be incompatible with the running kernel.

### Scenario C — DMA works on one architecture but corrupts data on another

Inspect structure size/offsets, required alignment, endian conversions, DMA mapping direction, DMA mask, cache synchronization, descriptor ownership and barriers. Do not assume CPU physical address equals DMA address. Do not fix a layout bug by adding __packed without checking the device specification and unaligned access rules.

### Scenario D — a built-in driver probes before its provider

Inspect Kconfig dependencies, initcall levels, object/link order, firmware descriptions and deferred-probe behavior. Do not rely on an accidental Makefile ordering if the framework has a proper dependency mechanism.

### Scenario E — an init-only function is called after boot

Search for stored function pointers and callbacks that reference __init code. Confirm that no long-lived object retains a pointer into a section reclaimed after initialization. Treat section annotations as lifetime constraints.

## 9. Exercises

- **Beginner:** Add a u16 field to the sample structure. Predict padding, then verify with sizeof() and offsetof(). Explain why the answer may differ on another ABI.
- **Intermediate:** Add an explicit little-endian field to a mock descriptor. Show where CPU-to-little-endian conversion belongs and why __le32 alone does not convert bytes.
- **Intermediate:** Change the module's Kbuild declaration to a composite module with separate core.o and hw.o objects. Keep the init/exit implementation in one object and verify the link.
- **Advanced:** Inspect vmlinux.lds.S and the initcall section macros for one architecture. Trace a built-in driver init function from its macro to the linker section and runtime iteration.
- **Advanced:** Reproduce an external-module build using a separate O= output tree. Record the kernel commit, .config, compiler version, architecture, Module.symvers source and exact build command.

## 10. Knowledge check

1. Why is dma_addr_t not interchangeable with phys_addr_t?
2. What is the difference between an ELF ET_REL object and the final linked kernel image?
3. What information does a relocation entry carry?
4. Why can a .ko be a relocatable ELF object?
5. What is the difference between obj-y and obj-m?
6. Why can changing built-in object order affect initialization?
7. Why are C bitfields a poor choice for describing MMIO register layouts?
8. What does __iomem communicate, and what does it not do?
9. Why does modules_prepare not always provide enough information for a module build with symbol versioning?
10. Why must you not load a module built against headers that do not match the running kernel?

## 11. Answer checkpoints

1. An IOMMU may translate the device-visible DMA address; the DMA API owns the mapping contract.
2. ET_REL needs relocation/linking; the final kernel image has its linked placement and architecture-specific layout.
3. It identifies a location and relocation type/symbol/addend needed to resolve an address or value later.
4. The module loader places its sections and applies architecture-specific relocations at load time.
5. obj-y contributes to the built-in kernel; obj-m creates a loadable module.
6. Link order can determine order within linker-collected initcall levels; explicit dependencies/levels are safer.
7. Bitfield layout and allocation order are implementation-defined and may not match hardware.
8. It marks an I/O address space for analysis; actual accesses still require the I/O API.
9. modules_prepare does not produce the full symbol-version table required by CONFIG_MODVERSIONS.
10. Vermagic, ABI, config, symbol versions and architecture may not match; compilation alone is not load compatibility.

## 12. References and source archaeology

Pinned Linux v7.4-rc4 sources:

- [Kernel programming language](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/process/programming-language.rst)
- [Kernel coding style](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/process/coding-style.rst)
- [Kbuild Makefiles](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/makefiles.rst)
- [External modules](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/modules.rst)
- [Kconfig language](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/kbuild/kconfig-language.rst)
- [Kernel types](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/types.h)
- [Compiler annotations](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/compiler_types.h)
- [Bitfield helpers](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/bitfield.h)
- [Byte-order helpers](https://github.com/torvalds/linux/blob/v7.4-rc4/include/linux/byteorder/generic.h)
- [Top-level Makefile](https://github.com/torvalds/linux/blob/v7.4-rc4/Makefile)
- [Kbuild object compilation](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/Makefile.build)
- [Module post-processing](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/Makefile.modpost)
- [Final vmlinux link](https://github.com/torvalds/linux/blob/v7.4-rc4/scripts/link-vmlinux.sh)
- [x86 linker script](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/kernel/vmlinux.lds.S)
- [Common linker-script definitions](https://github.com/torvalds/linux/blob/v7.4-rc4/include/asm-generic/vmlinux.lds.h)
- [ELF generic ABI](https://gabi.xinuos.com/)

Historical Kbuild examples are recorded in [SOURCE-ARCHAEOLOGY.md](../../SOURCE-ARCHAEOLOGY.md). Two useful cases are the initcall-order dependency fix in [commit 20dff140](https://github.com/torvalds/linux/commit/20dff14006b189baf05011b44e785d100458cc95) and the GNU Make jobserver/Clang LTO fix in [commit ae5f6044](https://github.com/torvalds/linux/commit/ae5f6044c4cdffe2b3361b15c1d6fc67b339f2a4). Both show why build-system details can affect runtime behavior and build correctness.

---

**Completion rule:** the userspace ELF lab and external module compile are separate validations. A successful userspace build does not validate a kernel module, and a successful module link does not prove that it is safe or compatible to load.
