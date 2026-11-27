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
