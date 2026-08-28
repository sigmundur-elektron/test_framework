---
last-updated: 2026-08-28
owner: copilot
status: active
---

# Decision Log (ADR-lite)

Append one entry per non-trivial decision. Newest at top.

---

## D-005 — Adopt `spec-flow`: a minimal moai-adk-style harness; Phase 1 = the quality gate
**Status:** accepted
**Date:** 2026-08-28

**Context.** [MoAI-ADK](https://github.com/modu-ai/moai-adk) is an agentic
development harness for Claude Code: a SPEC-driven `plan → run → sync` lifecycle,
TRUST 5 quality gates, evidence-bound completion claims, and — in v3.1 — a
five-column Kanban board spread across four hand-launched terminals. It is large
(12 agents, 18+ skills, an MCP server, a web console, multi-LLM cost routing).
Most of that machinery exists to work around Claude Code limitations that
opencode does not have; notably, opencode's `task` tool already gives each
subagent an isolated context, which is the whole reason Kanban Mode exists.

The recurring, concrete failure it addresses is real for us: an agent asserting
that tests pass without having run them.

**Decision.** Adopt a deliberately small port — `spec-flow` — in phases.

- **Phase 1 (this change, T-019).** The verification half only:
  `scripts/gate.ps1`, the `quality-gate` and `evidence-report` skills, the
  `/gate` command, and `opencode.json`. **No agents, no SPEC documents, no model
  routing.** The evidence discipline is testable on its own before any ceremony
  is added.
- **Phase 2 (T-027, deferred).** The SPEC half: a `spec-format` skill, one SPEC
  per feature branch under `.spec/`, `/plan` and `/run` commands, and a
  `spec-auditor` + `verifier` pair enforcing *author ≠ auditor*.
- **Explicitly dropped.** Kanban/Factory modes, worktree orchestration, MCP
  server, web console, multi-LLM cost routing, 16-language detection, the
  `@MX`/Navigator tag graph, the goal engine, and the self-improvement loop.

Scope is **project-local and committed** — everything lives in this repository
and is traceable by git. Agents must not write artefacts outside the worktree;
`opencode.json` sets `external_directory: deny`.

**The gate was established by measurement, not assumption.** The first draft of
the proposal guessed `cmake --preset` + `ctest --preset`; both are wrong here:

- `ctest` does not work at all — there is no `enable_testing()`/`add_test()`.
  Tests are doctest cases compiled *into* the app and run via `test.exe --test`.
- `CMakePresets.json` has configure presets only, so `cmake --build --preset`
  fails; the build dir must be named explicitly.
- Neither `cmake` nor `clang-format` is on `PATH`; both ship inside Visual
  Studio, and `cl.exe` needs the VS developer environment for `INCLUDE`/`LIB`.
- Exit code 0 is not a pass: `test/mvp_gaps_test.h` marks 4 known MVP gaps
  `may_fail`, so they print `ERROR:` and the run still exits 0.

Recorded baseline (`x64-debug`, 2026-08-28): build exit 0 / 0 first-party
warnings; 22 test cases, 22 passed; 97 assertions, 93 passed, 4 `may_fail`;
44 of 80 tracked `src/`+`test/` files fail `clang-format`.

**Rationale.**
- A gate expressed as *prose in agent instructions* is reassembled from natural
  language on every run and drifts. A script has one output format and an exit
  code, so its result is reproducible by a human.
- Because 44/80 files already fail formatting, the gate defaults to **changed
  files only** (`-Scope Changed`), with `-Scope Branch` for PR time. This lets
  the gate be green today without pretending the debt does not exist (T-021).
- Two skills and one command is a small enough surface that a wrong call is
  cheap to notice; a 52-skill catalogue is not.

**Impact.**
- New: `scripts/gate.ps1`, `.opencode/`, `opencode.json`,
  `docs/proposals/spec-flow.html`.
- Tracker: T-019 done; **T-020 … T-029** opened for every identified limitation.
- No application source changes.

**Consequences.** Completion claims now have a defined shape and a command that
backs them. The gate remains **advisory** — nothing blocks a commit that skipped
it (T-025 proposes a git `pre-commit` hook, which binds humans and agents alike,
rather than an opencode plugin, which would bind only agents). Phase 2 will add
two subagent round-trips per unit of work; whether the SPECs prevent enough
rework to pay for that is unmeasured, by us and by MoAI-ADK alike.

## D-004 — "Save as template" for agents; de-emphasize setup export; two-way export
**Status:** accepted
**Date:** 2025-02-16

**Context.** Review of the open questions (Q11–Q13). The front-and-center
"agent export" UI was judged bad framing and too prominent. Users want to reuse
an agent's configuration when creating new agents, and want export to be
round-trippable.

**Decision.**
- Add **`features::template_service`** (`src/features/agent_template.{h,cpp}`):
  save an agent's reusable config (endpoint, grants, peers — not the unique name
  or memory) as a versioned Glaze-JSON **template**, list templates, and create
  a new agent from a template.
- **Whole-setup export** stays on `agent_service::export_setup`, but its UI is
  **de-emphasized** (moved out of the front-and-center position in
  `agent_panel`).
- **Export becomes two-way** (revises D-003's one-way v1 decision): import /
  round-trip promoted into MVP scope (T-010, Q12).

**Impact.**
- New files `src/features/agent_template.h/.cpp`; template UI in `agent_panel`.
- New tests `test/agent_template_test.h` and MVP-gap markers in
  `test/mvp_gaps_test.h` (intentionally failing until the guarded MVP features
  land — planner, mcp_client, permissions, backends).

**Consequences.** Agent configs are reusable; the export button is no longer the
visual focal point; the test suite deliberately fails on unimplemented MVP
features so gaps stay visible.

## D-003 — Freeze export schema v1 (`tf.agent-export`) and implement on `agent_service`
**Status:** accepted
**Date:** 2025-02-15

**Context.** T-007/T-008 required freezing the portable export format and
implementing it. Open questions Q4/Q10/Q11/Q12/Q13 gated the freeze.

**Decision.**
- Format: **custom `tf.agent-export` JSON** serialized via Glaze (Q11).
- Backends: **excluded** — no database/backend endpoints in the export (Q10).
- Memory: **full snapshot included**, versioned by the document `version` (Q4).
- Direction: **one-way snapshot** for v1; import (T-010) deferred (Q12).
- Home: **`agent_service::export_setup`**, delegating to
  `features::build_export_document` + `export_document_to_json` (Q13).

**Impact.**
- New files `src/features/agent_export.h/.cpp`; UI Export button in
  `agent_panel`; tests in `test/agent_export_test.h`.
- `plans` array empty in v1 (no persisted authored plans yet); tool bindings
  derived from the live `tool_registry` with `kind: "local"`.

**Consequences.** A single command produces a portable artifact capturing
agents, configs, permissions, peers, tool bindings, and memory.

## D-001 — Coordination workspace folder name: `docs/`
**Status:** accepted
**Date:** 2025-02-14

**Context.** The user asked for a project-level folder to store AI
overviews/notes/plans so agents can coordinate. A plain `ai/` would collide
conceptually with the existing `src/data/` source tree.

**Decision.** Use a **root-level `docs/`** folder for coordination
artifacts.

**Rationale.**
- Clearly signals "tooling/coordination," not application source.
- Dotted, root-level location keeps it out of `file(GLOB_RECURSE src/*.cpp)`
  so it never affects the build.
- Avoids naming confusion with `src/data/` (project AI data).

**Alternatives considered.** `ai-workspace/`, `docs/ai/`, `.ai/` — rejected in
favor of the conventional `docs/` tooling namespace.

**Consequences.** All coordination docs live under `docs/`; conventions
defined in [`../README.md`](../README.md).
