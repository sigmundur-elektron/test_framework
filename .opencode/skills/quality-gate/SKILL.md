---
name: quality-gate
description: Use before claiming any work is complete, before committing, and whenever asked to build, run tests, run the gate, check formatting, or verify a change in this repository. Contains the exact verified commands, the current baseline numbers, and the pass thresholds for test_framework.
---

# Quality gate — test_framework

## Run it

```powershell
python scripts/gate.py
```

That script **is** the gate. Do not reassemble the steps by hand — the script
pins the toolchain, prints every command with its exit code, and ends with a
`GATE: PASS` / `GATE: FAIL` line. Hand-rolled equivalents drift and produce
evidence nobody can reproduce.

Useful variants:

| Command | When |
|---|---|
| `python scripts/gate.py` | default — format check covers files changed vs `HEAD` plus untracked |
| `python scripts/gate.py --scope branch` | before opening a PR — covers everything changed vs the base branch, auto-detected from `origin/HEAD` (this repo: `origin/master`). An unresolvable base ref **fails** rather than silently narrowing the scope. |
| `python scripts/gate.py --skip-format` | fast build+test loop while iterating |
| `python scripts/gate.py --reconfigure` | after editing `CMakeLists.txt` or `CMakePresets.json` |
| `python scripts/gate.py --clean` | suspected stale build; ~10 minutes, rebuilds all vendored deps |
| `python scripts/gate.py --scope all` | auditing repo-wide format debt only — **expected to fail**, see T-021 |
| `python scripts/gate.py --scope all --update-baseline` | re-record `scripts/baseline.json` after a deliberate change |
| `python scripts/gate.py --skip-self-test` | skip step 0; only when you are deliberately editing `gate.py` mid-debug |

## Validating the harness itself

```powershell
python scripts/check_opencode.py    # .opencode/ structure
python scripts/check_docs.py        # docs/ referential integrity
```

Both are separate from the code gate and are **not** run by `gate.py`; a typo in
a document should not fail a build.

`check_opencode.py` exists because opencode loads `.opencode/` once at startup
and fails *quietly* on several mistakes: a skill whose `name` does not match its
directory is dropped, an invalid `permission` key is silently routed into
`options`, and a missing `description` hides an agent or skill from the model
entirely. None of that raises an error you would notice mid-session.

`check_docs.py` enforces the conventions `docs/` runs on: every `T-NNN` cited
must exist as a tracker row, every `D-NNN` as a decision heading, relative links
must resolve, and measured numbers must not be hardcoded in prose. To cite an ID
that deliberately does not exist, suppress it explicitly:

```markdown
<!-- check-docs: allow-missing T-012 D-002 -->
```

A suppression for an ID that later starts existing warns, so it cannot rot.

Run both after editing anything under `.opencode/`, `opencode.json` or `docs/`,
and **restart opencode** after `.opencode/` changes — config is not hot-reloaded.

## What it runs

| # | Step | Command | Threshold |
|---|---|---|---|
| 0 | tooling | `python scripts/test_tooling.py` | exit 0. The gate's own unit tests. A failure here **stops the run** — a broken gate produces evidence that looks like a pass. ~0.02s. |
| 1 | configure | `cmake --preset x64-debug` | exit 0. Skipped when `out/build/x64-debug/CMakeCache.txt` exists. |
| 2 | build | `cmake --build out/build/x64-debug` | exit 0, **and zero new warnings in first-party code**. Warnings from `external/` are filtered out. |
| 3 | test | `out/build/x64-debug/test.exe --test` | exit 0, and the doctest counts must not regress against `scripts/baseline.json`. |
| 4 | format | `clang-format --dry-run -Werror` on in-scope files | exit 0. |

## Baseline — `scripts/baseline.json`

**Do not restate these numbers here or anywhere else.** They live in
`scripts/baseline.json`, and `gate.py` compares against them on every run,
printing one of these in the `GATE SUMMARY` block:

| Line | Meaning |
|---|---|
| `BASELINE: MATCH` | the run agrees with the recorded facts |
| `BASELINE: AHEAD (...)` | you improved something. Not a failure. Re-record. |
| `BASELINE: DRIFT (...)` | coverage dropped, `may_fail` changed, a warning appeared, or format debt grew. **Fails the gate.** |

Re-record deliberately, never silently:

```powershell
python scripts/gate.py --scope all --update-baseline
```

This file exists because the numbers were previously copied as prose into three
places and drifted three ways: `verifier.md` was told `may_fail` was 4 when it
was 3, so it reported a false finding on **every** run, and the format debt was
quoted as both 44/80 and 38/80 when it was 38/81. A number a machine checks
cannot drift; a number in a sentence always does.

To read the current values, open the file — do not memorise them.

## Things about this repo that will mislead you

- **`ctest` does not work here.** There is no `enable_testing()` or `add_test()`
  in `CMakeLists.txt`, and `CMakePresets.json` defines configure presets only —
  no build or test presets. `cmake --build --preset x64-debug` and
  `ctest --preset x64-debug` both fail. Tests run by invoking the app binary.
- **Tests are compiled into the application.** `src/main.cpp` defines
  `DOCTEST_CONFIG_IMPLEMENT` and `#include`s each `test/*_test.h` by hand. A new
  test file that is not added to that include list silently never runs. Check
  the include list when test counts look wrong. (See T-022.)
- **`test.exe` is a GUI app. Always pass `--test`.** `src/main.cpp` computes
  `run_tests = testing || wants_tests(argv)` where `testing` is a compile-time
  default (`src/main.cpp:16`). With `testing = false`, an unflagged run calls
  `a.run()`, opens a GLFW/ImGui window and **blocks indefinitely** — verified by
  running the binary with no arguments: no doctest output, still alive after 12s.
  Never invoke it without `--test` from a script or an agent session.
  Note the coupling: if `testing` is ever set back to `true`, the `||` can never
  be false and the suite runs regardless of the flag, making the GUI
  unreachable. The gate always passes `--test`, so it is correct either way.
- **Some assertions fail by design.** `test/mvp_gaps_test.h` marks known MVP gaps
  with doctest's `may_fail`, so they print `ERROR:` followed by
  `Allowed to fail so marking it as not failed`, and the run still exits 0.
  The gate reports this as `may_fail assertions: N` and checks N against
  `scripts/baseline.json`. **A change in that count fails the gate** — a drop can
  mean a gap was genuinely closed, or that someone deleted the marker, and only a
  human can tell which. (T-011 closed the permissions gap and moved the count.)
- **Exit code 0 alone is not a pass.** Because of the above, always read the
  doctest summary lines, not just the exit code.
- **`clang-tidy` is not part of the gate.** There is no `.clang-tidy` config and
  no `compile_commands.json`. Do not claim static analysis ran. (T-023.)
- **There is no coverage measurement.** Do not quote a coverage number. (T-024.)

## Reporting

Report the outcome with the `evidence-report` skill. Paste the script's
`GATE SUMMARY` block verbatim as the Evidence section — a gate result with no
command output attached is not a gate result.

If the gate fails on `format` for files you did not touch, say so; do not
reformat unrelated files to make the gate green.
