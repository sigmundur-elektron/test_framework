---
description: Implement the current branch's audited SPEC, then verify it independently
agent: build
---

Implement the SPEC for this branch. $ARGUMENTS

Current branch and working tree:
!`git rev-parse --abbrev-ref HEAD`
!`git status --short`

## Procedure

1. **Load the SPEC.** `<branch-slug>` is the branch name with `/` replaced by `-`.
   Read `.spec/<branch-slug>/spec.md` and `progress.md` in full.
   - No `spec.md` → stop, tell me to run `/plan` first. This is the only hard stop.
   - `status: draft` or `audited-with-debt` → **proceed**, but say so up front,
     list the known defects from `progress.md`, and treat any acceptance
     criterion they touch as suspect. Note it in `progress.md` before you start.
   - An unwritten `D-NNN`, a stubbed dependency or an open `T-NNN` is **not a
     blocker**. Work around it and record what you worked around.
   - `status: done` → ask what changed.

2. **Establish a starting point.** Run `python scripts/gate.py` once before
   touching anything. If the gate is already failing, say so and record it — you
   cannot attribute a failure to your change if it was failing beforehand. If the
   gate command itself is missing or stale, fix the reference and tell me, rather
   than reporting the tooling error as a baseline.

3. **Tests first.** Write the doctest cases from the acceptance criteria, in
   `test/*_test.h`, and add the include to `src/main.cpp` if the file is new
   (T-022 — nothing else will catch that omission). Run the gate and **paste the
   failing output**. Confirm they fail for the reason the SPEC predicts, not
   because they do not compile.

4. **Implement** until they pass. Touch nothing outside the SPEC's scope. If you
   find an unrelated defect, write it into `progress.md` and leave the code
   alone — that is a new task, not a free ride.

5. **Verify independently.** Dispatch @verifier. Do not run the gate yourself and
   present that as verification; you wrote the code, so your run is a claim and
   the verifier's is the evidence.

6. **Record.** Append the verifier's report to `.spec/<branch-slug>/progress.md`
   **verbatim**, under a dated heading. Do not summarise it.

7. **Iterate or stop.** If the verdict is a failure, fix and return to step 5.
   **Three rounds.** After that, stop and show me exactly where it stands, with
   the failing evidence — three failed verifications means something about the
   SPEC or the approach is wrong, and grinding costs more than asking. Partial
   progress is still progress: leave the tree in a state I can read, and record
   in `progress.md` which `A<n>` pass, which fail, and what you would try next.

8. When every `A<n>` is PASS, set `status: done` in the SPEC and update the
   relevant `T-NNN` in `docs/status/tracker.md`. Then tell me to run `/sync`.

## Rules

- Report to me using the `evidence-report` five-section format.
- **Do not claim completion without a verifier report showing `GATE: PASS`.**
  Not "the build looks fine", not "tests should pass" — the report or nothing.
- If an acceptance criterion turns out to be unverifiable under this repo's gate
  (no `ctest`, no `clang-tidy`, no coverage), say so plainly and mark it
  `NOT-CHECKED`. Do not quietly reinterpret it into something you can check.
- Do not reformat files the change did not touch to make the format step green.
- **You cannot commit.** `opencode.json` denies `git add`, `git commit`,
  `git push` and `git reset` to every agent except `integrator`, which only
  `/sync` runs. This is a permission boundary, not an instruction you can
  reason around: implementation does not land its own work. Finish, verify,
  then hand over to `/sync`.
