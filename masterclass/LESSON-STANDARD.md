# Lesson Standard

Every lesson follows this structure.

## 1. Learning objectives
State what the learner can explain, implement and debug.

## 2. Why this exists
Explain the engineering problem, constraints and design rationale. Separate documented history from interpretation.

## 3. Architecture
Describe components, ownership boundaries, data flow and control flow. Use sequence diagrams where ordering matters.

## 4. Kernel source walkthrough
- Pin the kernel release or commit.
- Link to exact files and functions.
- Explain call paths and important data structures.
- Identify locking, execution context, lifetime and error handling.
- Distinguish implementation details from stable ABI.

## 5. API guide
Explain calling context, ownership, return values, valid usage and common mistakes.

## 6. Hands-on lab
Include prerequisites, source files, Kconfig/Makefile where needed, build/run commands and expected output. Prefer QEMU or a disposable VM when hardware is not required.

## 7. Debugging exercise
Provide a realistic symptom, hypotheses, instrumentation, reproduction steps and a way to prove root cause.

## 8. Validation
Choose relevant build checks, Sparse/Smatch, Coccinelle, KUnit, kselftest, sanitizers, lockdep and hardware tests. State exactly which checks were run.

## 9. Exercises
- Beginner: explain or modify a small example.
- Intermediate: add an error path or concurrency condition.
- Advanced: analyze a real driver path or propose a minimal patch.

## 10. Knowledge check
Add 5–10 questions and a separate answer key.

## 11. References
Link kernel documentation, exact source files, subsystem maintainers and relevant specifications. Record source revision.

## Quality gates
- Never fabricate test results or bug claims.
- Never guess register offsets or hardware behavior.
- Do not direct experiments on production systems.
- Every resource acquisition has a clear release path.
- Every asynchronous callback has a documented lifetime and teardown story.
- Every DMA path explains mapping, synchronization, ordering and ownership.
- Every IRQ path explains acknowledgement, concurrency and teardown.
- Every PM path explains clocks, resets, power domains and failure recovery.
