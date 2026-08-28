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
     changed to `progress.md`, then re-audit. **Two rounds.**

7. **Close the audit either way.** `/plan` always produces a SPEC. It does not
   hand the work back because the loop did not converge.
   - If DEBT remains after round 2, apply whatever fixes you can, set
     `status: audited-with-debt`, and copy every unresolved defect **verbatim**
     into `progress.md` under `## Known defects at audit close`. A SPEC carrying
     two recorded defects is worth more than no SPEC.
   - **The one exception:** if the auditor's defects say the *request* is
     ambiguous rather than that the author got facts wrong, stop and ask me.
     Wrong facts are yours to fix; an unclear ask is mine.
   - Unwritten `D-NNN` entries, stubbed dependencies and open `T-NNN` are **not
     blockers**. Plan around them, name them in Context or Risks, and say what
     the implementer should do when they hit one.

8. **Learn from it.** If the audit found a defect class you have now hit twice,
   say so, and propose the rule or template change that would prevent a third.
   Add it to `docs/status/tracker.md`.

## Rules

- **Write no implementation code in this command.** None. Not a stub, not a
  signature, not a test. `/plan` produces one document and nothing else.
- **You cannot commit.** `opencode.json` denies `git add` and `git commit` to
  every agent except `integrator`, which only `/sync` runs. Planning does not
  land anything.
- Do not run the gate here; there is nothing to verify yet.
- **Write acceptance criteria that are checkable without a commit.** `/run`
  cannot commit either, so a criterion evaluated against `origin/master...HEAD`
  or any revision range is unverifiable at the moment it matters. Phrase it
  against the working tree, a doctest case, or a command's output instead.
- Reference the relevant `T-NNN` from `docs/status/tracker.md` in the SPEC's
  Context. If this work has no task, say so — it may need one.
- If the tier is L, note in the frontmatter which `D-NNN` should record the
  design decision. Write it if you can; if you cannot, say so and carry on —
  a missing decision record is a gap to report, not a gate to fail.
- **Name the tool, function or symbol exactly as the code spells it.** Acceptance
  criteria that name a registry key, a tool name or a test case must quote the
  literal string from the source, with a `file:line`. A criterion that passes for
  the wrong reason is worse than one that fails.
