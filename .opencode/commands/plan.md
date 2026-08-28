---
description: Author and independently audit a SPEC for the current feature branch
agent: build
---

Write a SPEC for: $ARGUMENTS

Current branch and working tree:
!`git rev-parse --abbrev-ref HEAD`
!`git status --short`

## Procedure

1. **Refuse to plan on a shared branch.** If the branch above is `main` or
   `master`, stop and tell me to create a feature branch first — one SPEC per
   feature branch is the whole convention. Suggest a name.

2. **Resolve the SPEC directory.** `<branch-slug>` is the branch name with `/`
   replaced by `-`; the SPEC lives at `.spec/<branch-slug>/`. If a `spec.md`
   already exists there, do not overwrite it — show it to me and ask whether to
   amend it or start over.

3. **Clarify before writing.** If the request is ambiguous, ask me now. One round
   of questions, then proceed. A SPEC written around a guess wastes the audit.

4. **Read the code first.** Use @explore to find what this touches. Do not
   describe current behaviour from memory — open the files. In this repo, check
   in particular whether the change needs a new `test/*_test.h`, because those
   are `#include`d by hand in `src/main.cpp` and are silently skipped otherwise
   (T-022).

5. **Write it.** Load the `spec-format` skill and follow the template exactly.
   Create `.spec/<branch-slug>/spec.md` and, beside it, `progress.md` opening
   with today's date and my request verbatim.
   Assign a tier honestly — when in doubt it is M.

6. **Audit it.** Unless the tier is S, dispatch @spec-auditor on the finished
   SPEC. You wrote it, so you do not grade it.
   - **PASS** → set `status: audited`, append the verdict to `progress.md`, and
     show me the SPEC path plus any nits.
   - **DEBT** → fix every blocking defect, append both the verdict and what you
     changed to `progress.md`, then re-audit. **Maximum two rounds.** If it still
     returns DEBT, stop and show me both verdicts — repeated DEBT usually means
     the request itself is unclear, and that is mine to resolve, not yours to
     paper over.

## Rules

- **Write no implementation code in this command.** None. Not a stub, not a
  signature, not a test. `/plan` produces one document and nothing else.
- Do not run the gate here; there is nothing to verify yet.
- Reference the relevant `T-NNN` from `docs/status/tracker.md` in the SPEC's
  Context. If this work has no task, say so — it may need one.
- If the tier is L, remind me that it also needs a `D-NNN` entry in
  `docs/notes/decisions.md`.
