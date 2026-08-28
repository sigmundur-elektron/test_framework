---
name: add-a-test
description: Use when adding, writing or modifying a test in test_framework - creating a new test/*_test.h, adding a doctest TEST_CASE, or wondering why a test you wrote is not running. Covers the manual src/main.cpp include step that silently drops entire test files, doctest conventions used here, and how to confirm the test actually executed.
---

# Adding a test — test_framework

## The one thing that will bite you

**Test files are `#include`d by hand in `src/main.cpp`.** There is no glob, no
auto-discovery, no registration macro. A `test/*_test.h` that is not in that
include list **compiles fine, runs nothing, and reports success.** Nothing in
the build, the gate or the test runner catches it. (T-022.)

If you add a file, you must add the line.

## Procedure

**1. Write `test/<subject>_test.h`.**

```cpp
#pragma once
#include "../src/data/tools/permissions.h"
#include <doctest/doctest.h>

TEST_CASE("[permissions] default construction denies all")
{
	const permissions perms;
	CHECK_FALSE(perms.allowed(permissions::scope::read_project));
}
```

- `#pragma once` — every header here uses it; no include guards.
- Include the code under test by **relative path from `test/`**: `../src/...`.
- Test-case names start with a bracketed tag: `[permissions]`, `[mvp-gap][mcp]`.
  The tag is how you filter and how a reader knows the subject.

**2. Add the include to `src/main.cpp`** — alphabetically, in the existing block:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT
#include "../test/agent_a2a_test.h"
#include "../test/agent_export_test.h"
...
#include "../test/permissions_test.h"     // <- yours
```

**3. Run the gate and confirm the count moved.**

```powershell
python scripts/gate.py
```

Look at two things, not the exit code:

- `test cases: N` in the summary — it must be **higher** than before.
- The `BASELINE:` line. Adding tests produces
  `BASELINE: AHEAD (test_cases N, was M)` and still `GATE: PASS`. That is the
  proof your file is wired in.

**If the count did not move, your include is missing.** That is the T-022
failure, and `AHEAD` not appearing is the only signal you get.

**4. Re-record the baseline** once the new count is intended:

```powershell
python scripts/gate.py --scope all --update-baseline
```

Do this deliberately. `scripts/baseline.json` is the single source of truth for
these numbers — never edit it by hand to make a run go green.

## Assertions used here

| Macro | Use |
|---|---|
| `CHECK(expr)` | non-fatal; the case continues. Default choice. |
| `CHECK_FALSE(expr)` | negative assertion, used heavily in `permissions_test.h` |
| `REQUIRE(expr)` | fatal; aborts the case. Use when later lines would crash. |
| `static_assert` | compile-time invariants — see below |

**Compile-time pinning.** `test/permissions_test.h:25-40` freezes the ordinals of
`permissions::scope` with `static_assert`, because those values are load-bearing
in three tables that are never compiled against each other. Copy that pattern
when a value's *wire representation* matters. Note the file also states, in a
comment, the limitation it cannot cover (an appended enumerator) rather than
pretending to. Do the same: a fake assertion is worse than a documented gap.

## Known-gap tests (`may_fail`)

`test/mvp_gaps_test.h` marks unimplemented MVP features with
`* doctest::may_fail()`. Those cases print `ERROR:` followed by
`Allowed to fail so marking it as not failed`, and **the run still exits 0**.

- The gate counts them and checks the count against `scripts/baseline.json`.
- **Changing that count fails the gate**, in either direction — a drop can mean
  the gap was genuinely closed, or that someone deleted the marker, and the
  output cannot distinguish those.
- When you implement a gap, remove its `may_fail` decorator *and* re-record the
  baseline in the same change.

## Facts about the runner

- **Tests are compiled into the application.** `src/main.cpp` defines
  `DOCTEST_CONFIG_IMPLEMENT` and owns `main()`.
- **`ctest` does not work.** No `enable_testing()`, no `add_test()`. Do not try.
- Run tests via the binary: `out/build/x64-debug/test.exe --test`, or just run
  the gate, which does it for you.
- **Never run `test.exe` without `--test`.** `run_tests = testing ||
  wants_tests(argv)` (`src/main.cpp:16,31`); with the current `testing = false`
  an unflagged run opens a GLFW/ImGui window and blocks indefinitely instead of
  running anything.
