---
description: Run the full quality gate and report the evidence
agent: build
subtask: true
---

Run the quality gate for this repository and report the result.

Working tree state:
!`git status --short`

Steps:

1. Load the `quality-gate` skill.
2. Run `python scripts/gate.py $ARGUMENTS`. Run it — do not describe what it
   would do, and do not substitute your own cmake/clang-format invocations.
3. Load the `evidence-report` skill and report in its five-section format. Paste
   the script's `GATE SUMMARY` block verbatim under Evidence.
4. Compare against the baseline recorded in the `quality-gate` skill. Call out any
   difference in test-case count, assertion count, or `may_fail` count explicitly.

Do not fix anything. This command reports; it does not repair. If the gate fails,
say precisely which step and why, and stop.
