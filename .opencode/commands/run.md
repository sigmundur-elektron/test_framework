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
   - No `spec.md` → stop, tell me to run `/plan` first.
   - `status: draft` on a Tier M or L SPEC → stop; it was never audited.
   - `status: done` → stop and ask what changed.

2. **Establish a starting point.** Run `python scripts/gate.py`
   once before touching anything. If the gate is already failing, say so and stop
   — you cannot attribute a failure to your change if it was failing beforehand.

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
   **Maximum three rounds.** After that, stop and show me exactly where it stands
   — three failed verifications means something about the SPEC or the approach is
   wrong, and grinding costs more than asking.

8. When every `A<n>` is PASS, set `status: done` in the SPEC and update the
   relevant `T-NNN` in `docs/status/tracker.md`.

## Rules

- Report to me using the `evidence-report` five-section format.
- **Do not claim completion without a verifier report showing `GATE: PASS`.**
  Not "the build looks fine", not "tests should pass" — the report or nothing.
- If an acceptance criterion turns out to be unverifiable under this repo's gate
  (no `ctest`, no `clang-tidy`, no coverage), say so plainly and mark it
  `NOT-CHECKED`. Do not quietly reinterpret it into something you can check.
- Do not reformat files the change did not touch to make the format step green.
- Do not commit. I commit.
