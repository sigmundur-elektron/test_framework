---
name: cpp-conventions
description: Use when writing, reviewing or reformatting C++ in test_framework - naming, headers, error handling, ownership, and the clang-format rules the gate enforces. Read before adding a class, changing a signature, or touching formatting, so a change does not fail the format step or drift from surrounding code.
---

# C++ conventions — test_framework

C++23 (`CMAKE_CXX_STANDARD 23`), MSVC, Windows. Match the surrounding file; this
document describes what the code already does, not an aspiration.

## Naming

Everything is `snake_case` — types included.

```cpp
struct agent_service { ... };          // types: snake_case
struct tool_result { bool success{}; };
void execute_step(const plan_step &s); // functions: snake_case
std::vector<scope> _granted;           // private members: leading underscore
```

- Types, functions, variables, files: `snake_case`.
- Private members: `_leading_underscore` (`permissions.h:32`).
- Interfaces are prefixed `i`: `itool`, `i_repository`.
- Tools are suffixed `_tool`, services `_service`, panels `_panel`,
  backends `_api`.

## Headers

- **`#pragma once`**, always. No include guards anywhere in this tree.
- `struct` is used even when there are private members
  (`permissions.h:6` is a `struct` with a `private:` section). Do not "correct"
  a `struct` to a `class` — it is the house style.
- Doc comments use `///` above the declaration.
- Includes are **sorted** (`SortIncludes: true`), project headers first as
  quoted relative paths, then `<system>` headers.

## Error handling

Fallible operations return `std::expected<T, std::string>`:

```cpp
virtual std::expected<tool_result, std::string> execute(const std::string &json_args,
														const permissions &perms) = 0;
```

- The error type is a human-readable `std::string`.
- **Make the message discriminating.** `permissions_test.h:91-94` asserts that a
  denial names the denied scope *and* does not say "unknown tool", because those
  are different failures reached through the same return path. A message that
  cannot distinguish two causes makes the test unable to either.
- Do not throw across layer boundaries.

## Ownership

- Prefer values. Use `std::move` into members (`permissions.h:24-27`).
- `explicit` on single-argument constructors.
- `const` on member functions that do not mutate (`allowed(scope) const`).
- Pass by `const &` for non-trivial types.
- Virtual destructors on interfaces (`itool.h:19` — `virtual ~itool() = default`).
- Do not reach for smart pointers by default; most ownership here is by value or
  by a registry that outlives its entries.

## Formatting — the gate enforces this

`.clang-format` is Microsoft-based with two settings that will surprise you:

| Setting | Value | Consequence |
|---|---|---|
| `UseTab` | **`true`** | **Indent with tabs, not spaces.** `TabWidth: 4` |
| `ColumnLimit` | **`0`** | No line-length limit. Do not hand-wrap to 80/100. |
| `BreakBeforeBraces` | `Custom` | Allman: brace on its own line after functions, classes, control statements |
| `PointerAlignment` | `Right` | `const std::string &s`, not `const std::string& s` |
| `SpaceBeforeParens` | `ControlStatements` | `if (x)` but `foo(x)` |

```cpp
bool permissions::allowed(scope s) const
{
	// Flat allow-list, deny by default. No wildcards, no hierarchy, no
	// deny-entries: a scope is permitted only if it was explicitly granted.
	return std::find(_granted.begin(), _granted.end(), s) != _granted.end();
}
```

Note the comment: it states what the policy *is not*, which is what a reader
needs. Comments here explain decisions, not mechanics.

**Do not reformat files your change did not touch.** 38 of 81 tracked files
under `src/` and `test/` are already non-conformant (T-021, current count in
`scripts/baseline.json`). The gate checks changed files only, by design.
Bulk-reformatting to make a run green destroys review signal and is explicitly
forbidden.

To fix a file you *did* touch:

```powershell
clang-format -i <file>     # ships inside Visual Studio; not on PATH
```

The gate prints the full path to the right `clang-format.exe` when it fails.

## Testing

Tests are doctest cases compiled into the app, and **new test files must be
`#include`d by hand in `src/main.cpp`** or they silently never run (T-022).
Load the `add-a-test` skill before adding one.

## What the compiler will not tell you

- The `permissions::scope` enum's order is load-bearing in three uncoupled
  tables; see the `architecture` skill before touching it.
- Warnings from `external/` are filtered out of the gate. First-party warnings
  are counted and a new one **fails** the gate against
  `scripts/baseline.json`.
