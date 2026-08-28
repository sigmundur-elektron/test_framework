---
name: quality-gate
description: Use before claiming any work is complete, before committing, and whenever asked to build, run tests, run the gate, check formatting, or verify a change in this repository. Contains the exact verified commands, the current baseline numbers, and the pass thresholds for test_framework.
---

# Quality gate — test_framework

## Run it

```powershell
pwsh -File scripts/gate.ps1
```

That script **is** the gate. Do not reassemble the steps by hand — the script
pins the toolchain, prints every command with its exit code, and ends with a
`GATE: PASS` / `GATE: FAIL` line. Hand-rolled equivalents drift and produce
evidence nobody can reproduce.

Useful variants:

| Command | When |
|---|---|
| `pwsh -File scripts/gate.ps1` | default — format check covers files changed vs `HEAD` plus untracked |
| `pwsh -File scripts/gate.ps1 -Scope Branch` | before opening a PR — covers everything changed vs `origin/main` |
| `pwsh -File scripts/gate.ps1 -SkipFormat` | fast build+test loop while iterating |
| `pwsh -File scripts/gate.ps1 -Reconfigure` | after editing `CMakeLists.txt` or `CMakePresets.json` |
| `pwsh -File scripts/gate.ps1 -Clean` | suspected stale build; ~10 minutes, rebuilds all vendored deps |
| `pwsh -File scripts/gate.ps1 -Scope All` | auditing repo-wide format debt only — **expected to fail**, see T-021 |

## What it runs

| # | Step | Command | Threshold |
|---|---|---|---|
| 1 | configure | `cmake --preset x64-debug` | exit 0. Skipped when `out/build/x64-debug/CMakeCache.txt` exists. |
| 2 | build | `cmake --build out/build/x64-debug` | exit 0, **and zero new warnings in first-party code**. Warnings from `external/` are filtered out. |
| 3 | test | `out/build/x64-debug/test.exe --test` | exit 0, and the doctest counts must not regress against the baseline below. |
| 4 | format | `clang-format --dry-run -Werror` on in-scope files | exit 0. |

## Baseline — measured 2026-08-28 on `x64-debug`

```
build   exit 0    0 first-party warnings   (incremental; see T-020 for clean-build warnings)
test    exit 0    test cases: 22 | 22 passed | 0 failed | 0 skipped
                  assertions: 97 | 93 passed | 4 failed
                  may_fail assertions: 4
format  44 of 80 tracked files under src/ and test/ are non-conformant (T-021)
```

Quote this baseline in the **Baseline attribution** section of any evidence
report. If your run produces different numbers, that difference is the finding
— report it, do not smooth it over.

## Things about this repo that will mislead you

- **`ctest` does not work here.** There is no `enable_testing()` or `add_test()`
  in `CMakeLists.txt`, and `CMakePresets.json` defines configure presets only —
  no build or test presets. `cmake --build --preset x64-debug` and
  `ctest --preset x64-debug` both fail. Tests run by invoking the app binary.
- **Tests are compiled into the application.** `src/main.cpp` defines
  `DOCTEST_CONFIG_IMPLEMENT` and `#include`s each `test/*_test.h` by hand. A new
  test file that is not added to that include list silently never runs. Check
  the include list when test counts look wrong. (See T-022.)
- **`test.exe` is a GUI app.** Without `--test` (or `-t`) it opens a GLFW/ImGui
  window and blocks. Always pass `--test`.
- **4 assertions fail by design.** `test/mvp_gaps_test.h` marks known MVP gaps
  with doctest's `may_fail`, so they print `ERROR:` followed by
  `Allowed to fail so marking it as not failed`, and the run still exits 0.
  The gate reports this as `may_fail assertions: 4`. **If that count changes,
  say so explicitly** — a drop can mean a gap was closed, or that someone
  deleted the marker.
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
