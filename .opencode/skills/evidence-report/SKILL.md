---
name: evidence-report
description: Use when reporting that a build, test suite, lint run, format check or any other verification passed or failed, and whenever closing out a task with a completion claim. Enforces the five-section evidence format and bans presenting unrun or stale checks as results.
---

# Evidence report

A claim is what you assert. Evidence is a command, its exit code, and its output.
Only the second one counts.

## The integrity rule

1. **Never present a check you did not run as a success.** "This should build" is
   not a build result.
2. **Never present a stale measurement as a fresh one.** If a number came from an
   earlier run, say which run and how old it is.
3. **Never wave through what you did not observe.** If you did not read the
   output, you do not know what it said.
4. **A check that could not run belongs under Gaps, never under Evidence.**
5. **Do not soften a failure.** "Mostly passing" is not a verdict. Report the
   failing step, its exit code, and stop.
6. **Do not reuse another agent's claim as your evidence.** If a subagent told you
   the tests passed, that is a claim. Run the gate yourself, or cite the subagent's
   verbatim output and attribute it.

## The format

Every completion claim uses exactly these five sections, in this order.

### Claim
One sentence. What you assert is now true.

### Evidence
For each check: the exact command, its exit code, and the relevant output.
Paste real output. Do not paraphrase it.

```
$ python scripts/gate.py
=== GATE SUMMARY ==================================================
PASS   configure  exit 0    skipped: CMakeCache.txt present
PASS   build      exit 0    0 first-party warning(s)
PASS   test       exit 0    test cases: 22 | 22 passed | 0 failed | 0 skipped ;;
                            assertions: 97 | 93 passed | 4 failed ;; may_fail assertions: 4
PASS   format     exit 0    0/3 non-conformant
GATE: PASS
```

### Baseline attribution
What the numbers were before your change, and where that baseline came from.
The recorded baseline for this repo lives in the `quality-gate` skill. If you did
not measure a baseline, write **"no baseline measured"** — do not imply an
improvement you cannot show.

### Gaps
Checks that were skipped, could not run, or ran only partially, and why. Be
specific. Examples that belong here for this repo: clang-tidy (no config),
coverage (not instrumented), clean-build warnings (incremental build only),
format conformance outside the changed-file scope.

### Residual risk
What could still be broken that the checks above would not have caught. "None"
is only acceptable when the gate genuinely covers the surface you changed —
for a GUI, threading, or persistence change it almost never does.

## Worked example of a failure report

> **Claim** — The format step of the gate fails; build and tests pass.
>
> **Evidence**
> ```
> $ python scripts/gate.py
> PASS   build      exit 0    0 first-party warning(s)
> PASS   test       exit 0    22 passed | 0 failed ;; may_fail assertions: 4
> FAIL   format     exit 1    1/1 non-conformant
>   src/main.cpp
> GATE: FAIL (format)
> ```
>
> **Baseline attribution** — Matches the recorded baseline for build and test
> (22 cases / 97 assertions / 4 may_fail). Format has no clean baseline: 44 of 80
> tracked files were already non-conformant before this change (T-021).
>
> **Gaps** — clang-tidy not run (no `.clang-tidy`, no `compile_commands.json`).
> No coverage measurement. Build was incremental, so warnings in already-compiled
> translation units were not re-emitted.
>
> **Residual risk** — `src/main.cpp` is the doctest entry point; a formatting fix
> there cannot change behaviour, but the file is also where test headers are
> included by hand, so an edit could drop a test file from the run without any
> check catching it.
