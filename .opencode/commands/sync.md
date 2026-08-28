---
description: Close out finished work on this branch - verify the gate, update the tracker, stage explicitly and commit
agent: integrator
---

Close out the work on this branch. $ARGUMENTS

Current state:
!`git rev-parse --abbrev-ref HEAD`
!`git status --short`
!`git log --oneline -3`

## Procedure

1. **Refuse on a shared branch.** If the branch above is `master` or `main`,
   stop and say so. Work reaches those by merge, not by a direct commit.

2. **Run the gate yourself.** `python scripts/gate.py --scope branch`. Do not
   accept a result someone pasted for you, and do not substitute your own cmake
   or clang-format calls. Read the `BASELINE:` line as well as the exit code.
   Also run `python scripts/check_opencode.py` if the change touches
   `.opencode/` or `opencode.json`, and `python scripts/check_docs.py` if it
   touches `docs/`.

3. **Stop if anything is red.** Report the failing step, its exit code and its
   output, and commit nothing. You close out work; you do not repair it.

4. **Bring the paperwork up to date.** Move the `T-NNN` row in
   `docs/status/tracker.md`, record a `D-NNN` in `docs/notes/decisions.md` for
   any non-trivial choice, prune anything now answered from
   `docs/notes/open-questions.md`, and set `status: done` in
   `.spec/<branch-slug>/spec.md` if a SPEC exists. If a choice was made and you
   cannot write its decision entry, say so rather than skipping it silently.

5. **Stage explicitly, by path.** Never `git add -A`, `.` or `-u`. Then show
   `git status --short` and confirm that anything left unstaged was left
   deliberately. A working tree often holds edits that are not part of this
   work — leave them alone and name them in your report.

6. **Commit.** Imperative subject under 72 characters, then a body explaining
   why, referencing the relevant `T-NNN`/`D-NNN`. Where you claim something was
   verified, say what verified it.

7. **Report** in the `evidence-report` five-section format, and end with a **PR
   body** ready to paste. State plainly that nothing was pushed.

## Rules

- **This is the only command that commits.** `/plan` and `/run` are denied
  `git add` and `git commit` by `opencode.json`; that is deliberate, so that
  planning and implementation cannot quietly land work.
- **Never push, amend, force, rebase, merge or reset.** You are denied those by
  permission. Do not ask to be granted them.
- Do not reformat files this work did not touch to make the format step green.
- If the gate fails, the correct outcome is a clear report and no commit.
