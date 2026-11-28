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


## 12. Upstream change and trend analysis
For lessons derived from recent kernel development:
- State the exact kernel release, tag or commit and whether the change is merged mainline, present only in linux-next, proposed on a mailing list, or merely discussed.
- Link the merge commit, original subsystem pull request and relevant source diff.
- Explain the problem before the change, design alternatives, compatibility impact and why maintainers accepted it when evidence is available.
- Trace changed structures, functions, call paths, Kconfig and user-visible behavior.
- Include hardware/driver implications, regression risks, test strategy and a small hands-on exercise.
- Clearly label trend observations as confirmed merged work, active development, or informed interpretation.
- Compare with the prior implementation when useful; never present a proposed patch as released functionality.

## 13. Interview bank maintenance
Every lesson adds at least 10 non-duplicate questions: 4 fundamentals, 3 source/API/implementation, 2 debugging scenarios and 1 architecture/trade-off question. Include difficulty, concise answer checkpoints and a link to the lesson. Grow the bank toward 500+ questions over time; do not claim a target count until the questions exist.

## 14. Historical/source archaeology
Use git log, git blame, merge commits, subsystem pull requests, lore.kernel.org discussions and maintainer documentation. Record direct evidence for historical claims and label interpretation separately.


## Ownership and lifetime quality gate

For every asynchronous or shared driver object, include an ownership graph or table identifying creator, registry, open-file/stream, callback and hardware owners. Explain pointer publication/lookup protection, the exact get/put pairing, final-release context, probe failure unwind, remove/unbind behavior and the point at which DMA/firmware access is quiesced. Explicitly distinguish devres lifetime, object references, RCU grace periods, module-code lifetime and hardware ownership. Do not accept a successful compile as evidence that a teardown race is solved.
