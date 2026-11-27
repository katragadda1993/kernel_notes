# Lesson 01 — The User/Kernel Boundary: Privilege, System Calls, Exceptions and Interrupts

**Stage:** 0 — Systems foundations  
**Duration:** 60–75 minutes  
**Level:** Beginner → intermediate, with architecture-level source reading  
**Source revision:** Linux mainline tag [v7.4-rc4](https://github.com/torvalds/linux/tree/v7.4-rc4)  
**Lab:** [syscall_lab.c](lesson-01-user-kernel-boundary/syscall_lab.c) · [Makefile](lesson-01-user-kernel-boundary/Makefile)

## 1. Learning objectives

After this lesson you should be able to:

1. Explain why an operating system separates unprivileged applications from privileged kernel code.
2. Distinguish a system call, synchronous exception, asynchronous interrupt and context switch.
3. Describe x86-64 CPL3 → CPL0 and arm64 EL0 → EL1 transitions.
4. Trace one system call through architecture entry assembly, C dispatch and the return-to-user path.
5. Explain why a device interrupt is not the same thing as a system call.
6. Use a small C program and tracing tools to observe real system calls.

## 2. Why this boundary exists

A process must not be allowed to reprogram page tables, disable interrupts, access arbitrary physical memory, change device registers or overwrite another process's memory. If ordinary application code could execute those operations directly, one faulty application could corrupt the entire machine.

The CPU therefore provides privilege states and memory-access controls. Linux runs ordinary applications with restricted privilege and performs protected operations in kernel context.

The boundary is not simply a software convention. It is enforced by CPU privilege checks, page-table permissions, exception-entry rules and kernel validation.

### A useful mental model

~~~text
+-------------------------------+
| User process                  |
| libc, application, runtime    |
| Restricted CPU privilege     |
+---------------+---------------+
                |
        syscall / exception
                |  controlled entry
                v
+-------------------------------+
| Linux kernel                  |
| VFS, scheduler, memory,       |
| driver frameworks, drivers   |
| Privileged CPU execution     |
+---------------+---------------+
                |
        MMIO / DMA / IRQ
                v
+-------------------------------+
| Hardware: CPU, SoC IP, device |
+-------------------------------+
~~~

A user process requests a service; it does not receive unrestricted access to kernel internals. The kernel validates the request, checks permissions and object state, and invokes the appropriate subsystem.

## 3. CPU privilege states

| Architecture | Application context | Typical Linux kernel context | Other levels |
|---|---|---|---|
| x86-64 | CPL 3 | CPL 0 | CPL 1 and 2 exist architecturally but are not the normal Linux user/kernel split |
| arm64 | EL0 | EL1 | EL2 is used for hypervisor/virtualization; EL3 is associated with secure monitor firmware |

Important qualifications:

- A virtual-machine guest kernel commonly runs at EL1 while a hypervisor owns EL2. Host configurations can use virtualization extensions differently.
- ARM TrustZone secure/non-secure state is not interchangeable with the EL0/EL1 privilege distinction.
- Linux does not use x86 ring 1 and ring 2 as ordinary application privilege levels.
- Kernel virtual mappings and user virtual mappings are controlled by the MMU. A kernel address appearing in a page-table layout does not mean user code is permitted to access it.
- Features such as x86 Kernel Page-Table Isolation (KPTI) alter which mappings are present while crossing the boundary. They do not remove the need for privilege checks.

## 4. Four events that are often confused

### System call

A deliberate request by a program for a kernel service. Examples include read, write, openat, mmap and getpid. A system call is entered using an architecture-defined instruction and is handled as a synchronous exception/trap.

### Synchronous exception

An event associated with the instruction currently being executed. Examples include a page fault, undefined instruction, alignment fault or the system-call instruction itself. The CPU records enough state for the kernel to identify the event and decide whether to handle it, signal the process or terminate it.

### Asynchronous interrupt

An event that arrives independently of the instruction stream, commonly from a timer or peripheral. The CPU suspends the current execution at an architecturally valid point and enters an interrupt handler. A NIC, NVMe controller, USB host controller or audio DMA engine may raise an interrupt.

### Context switch

A scheduler operation that stops one task and resumes another. It may happen after an interrupt or while handling a system call, but it is not synonymous with either event. A system call can complete without switching to another task.

| Event | Initiator | Timing | Example |
|---|---|---|---|
| System call | Current program | Synchronous | write() |
| Exception | Current instruction or CPU condition | Synchronous | Page fault |
| IRQ | Timer or device | Asynchronous | SoundWire controller interrupt |
| Context switch | Scheduler | Scheduling decision | Task A → Task B |

A system call is not a hardware IRQ. Both can enter privileged code, but they have different causes, entry vectors and execution rules.

## 5. x86-64 system-call path

Use the exact v7.4-rc4 source, not a generic diagram from an old kernel.

### 5.1 User-side ABI

For the native x86-64 Linux syscall ABI:

| Purpose | Register |
|---|---|
| System-call number | RAX |
| Argument 0 | RDI |
| Argument 1 | RSI |
| Argument 2 | RDX |
| Argument 3 | R10 |
| Argument 4 | R8 |
| Argument 5 | R9 |
| Return address saved by SYSCALL | RCX |
| Saved flags | R11 |
| Return value | RAX |

The fourth argument is in R10, not RCX. The SYSCALL instruction uses RCX and R11 for the return RIP and flags, so the Linux syscall ABI differs from the ordinary x86-64 C function-call ABI.

### 5.2 Entry sequence

The important source is [arch/x86/entry/entry_64.S at v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/entry/entry_64.S#L87-L170).

Conceptually:

1. User code executes SYSCALL.
2. The CPU loads the configured kernel code segment and entry RIP from model-specific registers, saves the user RIP in RCX and flags in R11.
3. SYSCALL does **not** automatically switch RSP to a safe kernel stack.
4. Linux entry assembly performs the required GS handling, saves the user stack pointer, switches to the kernel stack and, when configured, handles the page-table switch for KPTI.
5. It builds the register frame used by the rest of the kernel.
6. It calls do_syscall_64().

That third point is a frequent source of misunderstanding: the CPU privilege transition is not a complete Linux entry path. The architecture assembly must establish a safe kernel execution environment.

### 5.3 Dispatch and return

See [arch/x86/entry/syscall_64.c at v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/entry/syscall_64.c#L24-L120).

- do_syscall_64() performs user-entry work and dispatches the requested operation.
- In this source revision, x64_sys_call() uses a switch generated from the syscall definitions. The source explicitly says sys_call_table[] is no longer used for dispatch; it remains available for syscall tracing metadata.
- The syscall-number definitions are generated from [arch/x86/entry/syscalls/syscall_64.tbl](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/entry/syscalls/syscall_64.tbl).
- syscall_exit_to_user_mode() performs exit work before user execution resumes.
- The fast SYSRET path is used only when the saved register state meets its constraints. Otherwise the kernel uses the more general IRET return path.

Do not teach “the kernel always indexes sys_call_table[]” as a universal fact. Dispatch implementation is architecture- and revision-specific.

## 6. arm64 system-call path

The arm64 path uses Exception Levels rather than x86 privilege rings.

### 6.1 User-side ABI

For native arm64 Linux:

| Purpose | Register |
|---|---|
| System-call number | X8 |
| Arguments | X0–X5 |
| Return value | X0 |

User code executes SVC #0. The processor takes a synchronous exception from EL0 to EL1. The exception mechanism records the return PC and saved processor state in architectural registers; Linux entry assembly saves the general-purpose register context.

### 6.2 Vector and handler sequence

Relevant sources:

- [arch/arm64/kernel/entry.S](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/entry.S#L208-L347) — entry-frame setup macros.
- [arch/arm64/kernel/entry.S](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/entry.S#L524-L626) — exception vectors and entry-handler generation.
- [arch/arm64/kernel/entry-common.c](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/entry-common.c#L791-L925) — EL0 synchronous and IRQ handling.
- [arch/arm64/kernel/syscall.c](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/syscall.c#L33-L150) — syscall-number extraction and dispatch.

The path is:

~~~text
EL0 application
  |
  | SVC #0; syscall number in X8
  v
EL0 synchronous vector
  |
  v
el0t_64_sync_handler()
  |
  | Decode ESR_EL1 exception class
  v
el0_svc()
  |
  v
arm64 syscall-entry work
  |
  v
do_el0_svc()
  |
  v
el0_svc_common()
  |
  v
invoke_syscall() -> syscall table entry
  |
  v
syscall return work -> EL0
~~~

The synchronous handler also handles other events, including instruction/data aborts and undefined instructions. It reads ESR_EL1 and selects the appropriate handler. A device IRQ takes a separate EL0 IRQ vector and reaches el0t_64_irq_handler(), which enters IRQ context and invokes the architecture IRQ handling path.

This is why a page fault and a syscall can share the broad synchronous-exception entry machinery without being the same event.

## 7. Return-to-user work is part of correctness

Returning to user mode is not merely restoring registers and executing SYSRET or ERET.

Before user execution resumes, Linux may need to:

- deliver a pending signal;
- restart or complete a syscall;
- schedule another task if rescheduling is required;
- complete tracing, uprobes or other deferred user-return work;
- update context-tracking and accounting state.

The generic return-work loop is visible in [kernel/entry/common.c at v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/entry/common.c#L1-L100).

For driver engineers this matters because an interrupt, completion, wakeup or signal can change what happens before a process resumes. Never assume that the instruction immediately after a syscall executes at the exact moment the driver completes its work.

## 8. Why the entry code is complicated: source archaeology

The x86 entry documentation says that its explanation was adapted from an Ingo Molnar email from 2011 and describes multiple entry mechanisms: native 64-bit SYSCALL, compatibility INT 0x80, 32-bit SYSENTER, IDT interrupts and architectural exceptions.

Read:

- [Documentation/arch/x86/entry_64.rst at v7.4-rc4](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/x86/entry_64.rst)
- [The historical entry explanation referenced by that document](https://lore.kernel.org/r/20110529191055.GC9835%40elte.hu)

The documented engineering lesson is that entry mechanisms have different hardware-created frames, stack behavior and calling conventions. Linux must normalize those differences without corrupting state.

The same document explains why SWAPGS handling is fragile: an entry can interrupt code that is already in the kernel, so blindly swapping GS on every entry is incorrect. NMI/debug-like paths require additional care because they can arrive at awkward points in the transition.

**Interpretation:** the complexity is not accidental boilerplate. It is the accumulated cost of supporting multiple CPU entry mechanisms, compatibility ABIs, virtualization, tracing, speculation mitigations, page-table isolation and precise return semantics. Keep that interpretation separate from the historical facts documented by the source.

## 9. Driver-engineering connection

A typical userspace-to-device flow is:

~~~text
Application
  |
  | read / write / ioctl / poll / mmap
  v
System-call entry
  |
  v
VFS or subsystem (ALSA, DRM, netdev, I2C, ...)
  |
  v
Driver callback
  |
  | MMIO programming / DMA queueing
  v
Hardware executes asynchronously
  |
  | IRQ or completion
  v
Kernel IRQ/completion path
  |
  | wakeup / poll readiness / completion state
  v
Application resumes
~~~

For an audio example, userspace submits PCM data through ALSA. The kernel ALSA/ASoC path eventually programs DMA and the audio hardware. The DMA engine transfers samples independently of the CPU. A period interrupt or another completion mechanism updates the PCM state and wakes a waiting task. The application does not directly execute the driver's interrupt handler.

For PCIe/NVMe, the same distinction applies: userspace submits I/O, the kernel driver posts commands to a hardware queue, the device performs DMA, and an MSI/MSI-X interrupt may report completion.

A system call is the request path. A hardware interrupt is often part of the completion path. They are separate events joined by kernel state and synchronization.

## 10. Hands-on lab

### 10.1 Build and run

The lab contains a libc syscall wrapper, one raw architecture syscall and an intentionally invalid syscall number.

~~~bash
cd masterclass/lessons/00-foundations/lesson-01-user-kernel-boundary
make
./syscall_lab
~~~

Expected output shape (PID varies):

~~~text
libc syscall() getpid: 1234
raw architecture syscall getpid: 1234
invalid syscall: ret=-1 errno=38 (Function not implemented)
~~~

The raw path uses SYSCALL on x86-64 and SVC #0 on arm64. Other architectures use the libc wrapper fallback.

### 10.2 Observe the syscall boundary

Install strace using your distribution's package manager if it is not already available:

~~~bash
strace -f -e trace=getpid,write ./syscall_lab
~~~

Look for getpid and write. Exact write grouping depends on libc buffering and terminal/output redirection. The invalid syscall should be reported as ENOSYS on a normal unrestricted Linux process.

### 10.3 Inspect architecture and interrupt counters

~~~bash
uname -m
gcc -dumpmachine
cat /proc/interrupts
~~~

On a disposable development system, run this in another terminal and observe which interrupt counters change while devices are active:

~~~bash
watch -n 1 cat /proc/interrupts
~~~

Do not expect identical interrupt names or counts across machines. The purpose is to observe that hardware interrupts are asynchronous and device/platform-specific.

Optional, if perf is installed and permissions allow:

~~~bash
perf stat -e page-faults,context-switches ./syscall_lab
~~~

### 10.4 What this lab proves—and what it does not

- It demonstrates a real user-to-kernel syscall transition and the raw syscall ABI on the architecture where it is compiled.
- It does not load a kernel module or touch device registers.
- It does not prove the arm64 inline-assembly branch works on x86. Build and run it on an arm64 machine to validate that branch.
- strace and perf are optional tools; their availability and permissions depend on the host.

## 11. Debugging exercises

### Scenario A — clock_gettime does not appear in strace

Possible explanation: the vDSO may satisfy the call in userspace without entering the kernel. Confirm by comparing a normal clock_gettime call with an explicit syscall(SYS_clock_gettime, ...). Do not conclude that tracing is broken from one missing syscall.

### Scenario B — a program gets ENOSYS

Check the syscall number and ABI for the running architecture. Confirm the program is using the correct native or compatibility ABI. Also inspect whether seccomp is active; a filter can deliberately synthesize an errno or terminate a process. ENOSYS alone does not prove the kernel lacks a feature.

### Scenario C — a driver operation works until a device interrupt arrives

Do not treat the IRQ as a continuation of the userspace syscall. Inspect the driver's shared state, locking, completion/wakeup path and object lifetime. Verify that IRQ context does not call a sleeping API and that deferred work is synchronized during remove.

### Scenario D — a return path unexpectedly uses IRET instead of SYSRET

Inspect the saved register frame and the architecture return constraints. A slower return path can be required for correctness; do not “optimize” it by forcing SYSRET without proving all state invariants.

## 12. Common mistakes

- Assuming a syscall is a hardware interrupt.
- Assuming every C library function enters the kernel.
- Assuming SYSCALL automatically switches the x86 stack.
- Putting the fourth x86-64 syscall argument in RCX instead of R10.
- Using x86 syscall numbers on arm64 or vice versa.
- Assuming a context switch happens on every syscall.
- Assuming every exception is a fault that should kill the process.
- Assuming a driver ISR runs in the context of the userspace process that initiated the I/O.
- Treating a kernel source implementation detail as a stable userspace ABI.

## 13. Exercises

**Beginner**

1. Draw the user → syscall → kernel → return path for getpid().
2. Explain the difference between CPL3/CPL0 and EL0/EL1.
3. Explain why an IRQ can arrive while a process is executing an ordinary instruction.

**Intermediate**

4. Read the x86 entry assembly and identify where user RSP is saved and where the kernel stack is selected.
5. Compare the x86-64 syscall argument registers with the arm64 syscall ABI.
6. Explain why the arm64 synchronous handler must inspect ESR_EL1 before deciding whether to run syscall dispatch.

**Advanced**

7. Explain how KPTI changes the x86 entry/exit sequence and why the transition must be carefully ordered.
8. Propose a trace plan for a userspace audio write that queues DMA and completes by interrupt. Separate syscall latency, DMA duration, IRQ latency and wakeup latency.
9. Explain why a driver must not trust a userspace pointer simply because the request arrived through a valid syscall.

## 14. Knowledge check

1. Is a system call an asynchronous interrupt? Why or why not?
2. Which x86-64 register contains syscall argument 4?
3. Which arm64 register contains the syscall number?
4. Does x86-64 SYSCALL switch RSP automatically?
5. Why can a syscall return path perform scheduling or signal work?
6. What is the difference between an exception and a context switch?
7. Why might clock_gettime not appear in strace?
8. Why should a driver not run its full completion path in hard-IRQ context?

### Answer checkpoints

1. No. It is a synchronous, deliberate transition initiated by the executing program.
2. R10.
3. X8.
4. No. Linux entry assembly establishes the kernel stack.
5. Signals, rescheduling and deferred return-to-user work must be handled before user execution resumes.
6. An exception is an event/entry condition; a context switch is the scheduler changing the running task.
7. The vDSO may implement it in userspace.
8. Hard IRQ context cannot sleep; long or blocking work must be deferred.

## 15. References

### Pinned kernel source

- [x86-64 syscall entry](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/entry/entry_64.S)
- [x86-64 syscall dispatch](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/entry/syscall_64.c)
- [x86-64 syscall-number definitions](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/x86/entry/syscalls/syscall_64.tbl)
- [arm64 exception entry assembly](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/entry.S)
- [arm64 EL0 entry handlers](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/entry-common.c)
- [arm64 syscall dispatch](https://github.com/torvalds/linux/blob/v7.4-rc4/arch/arm64/kernel/syscall.c)
- [generic entry and return work](https://github.com/torvalds/linux/blob/v7.4-rc4/kernel/entry/common.c)
- [x86 kernel entry documentation](https://github.com/torvalds/linux/blob/v7.4-rc4/Documentation/arch/x86/entry_64.rst)

### Reading discipline

Record the kernel tag, architecture, compiler and configuration whenever reproducing entry behavior. Entry assembly changes frequently because of mitigations, tracing, virtualization and architecture support. Do not copy an old entry sequence into a new kernel without checking the target revision.
