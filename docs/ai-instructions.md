# AI Instructions — test_framework

Loaded into every opencode session via `instructions` in
[`../opencode.json`](../opencode.json). Keep it short; detail belongs in
`.opencode/skills/` and [`README.md`](README.md).

## Project guidelines

- Prefer upgrading to the latest C++ standard that remains compatible and
  buildable. The project is currently on **C++23** (`CMAKE_CXX_STANDARD 23`).

## Working rules

These four are not optional. They exist because agents fail in these specific
ways, not as ceremony.

1. **Everything inside the repo.** Never create or modify files outside this
   worktree. Proposals, reports, scripts and agent config are committed and
   traceable by git. Mechanically enforced by `external_directory: deny`.

2. **Evidence, not claims.** Before asserting that a build, test run or check
   passed, run the gate and report with the five-section format:

   ```
   python scripts/gate.py      # or /gate
   ```

   A check that was not run belongs under **Gaps**, never under **Evidence**.
   Never present a stale measurement as a fresh one. Never reuse another agent's
   claim as your evidence. See `.opencode/skills/evidence-report/SKILL.md`.

   After editing anything under `.opencode/` or `opencode.json`, also run
   `python scripts/check_opencode.py` — opencode fails quietly on a
   skill/directory name mismatch, a bad frontmatter type or a missing
   description — then restart opencode, because config is not hot-reloaded.
   First time only: `python -m pip install -r scripts/requirements.txt`.

3. **Author ≠ auditor.** Whoever writes a SPEC does not grade it — `@spec-auditor`
   does. Whoever writes code does not verify it — `@verifier` does. Both are
   read-only by permission, not by convention. If you find yourself grading your
   own work, you have skipped a step.

4. **SPEC before code, on a feature branch.** Non-trivial work starts with
   `/plan`, which writes `.spec/<branch-slug>/spec.md`. Then `/run`. Trivial
   single-file changes may skip it — but claim Tier S on evidence, not on
   convenience.

## Traps in this repository

Read `.opencode/skills/quality-gate/SKILL.md` before running anything. Briefly:

- **`ctest` does not work.** No `enable_testing()`/`add_test()`. Tests are doctest
  cases compiled *into* the app: `out/build/x64-debug/test.exe --test`.
- **`cmake --build --preset` fails.** `CMakePresets.json` has configure presets
  only.
- **Exit code 0 is not a pass.** `test/mvp_gaps_test.h` marks 4 known MVP gaps
  `may_fail`; they print `ERROR:` and the run still exits 0. Read the doctest
  summary lines.
- **New test files are `#include`d by hand** in `src/main.cpp`. Forget that and
  the tests silently never run, and nothing catches it (T-022).
- **44 of 80 files fail `clang-format`** (T-021). The gate checks changed files
  only. Do not reformat unrelated files to make it green.

## Coordination

| | |
|---|---|
| Task board — single source of truth | [`status/tracker.md`](status/tracker.md) |
| Decision log (ADR-lite) | [`notes/decisions.md`](notes/decisions.md) |
| Open questions | [`notes/open-questions.md`](notes/open-questions.md) |
| Workspace conventions | [`README.md`](README.md) |
| Harness design + rationale | [`proposals/spec-flow.html`](proposals/spec-flow.html) |

Claim a task by its `T-NNN` before starting. Record non-trivial choices as a
`D-NNN`. Bump `last-updated` when you change a doc.

## Architecture

```
Agent Layer      reasoning · planning · task decomposition · memory
    │ agent-specific tools
Tool / Workflow  domain ops · validation · permissions · orchestration · MCP
    │ application APIs
Backends         Database · GitHub · Project
```

```
src/data/
├─ agent/
│  ├─ agent.h / agent.cpp            // top-level orchestrator (lifecycle)
│  ├─ planner.h                      // task decomposition / reasoning
│  └─ memory.h                       // context + short/long-term memory
├─ tools/
│  ├─ itool.h                        // tool interface (execute + schema)
│  ├─ tool_registry.h                // singleton, like input_manager
│  ├─ permissions.h                  // permission checks
│  └─ mcp/
│     └─ mcp_client.h                // MCP tool transport
└─ backends/
   ├─ database_api.h                 // normal app API
   ├─ github_api.h
   └─ project_api.h
```

Per-layer detail: [`overview/`](overview/).
