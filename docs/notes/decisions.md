---
last-updated: 2026-08-28
owner: copilot
status: active
---

# Decision Log (ADR-lite)

Append one entry per non-trivial decision. Newest at top.

---

## D-007 — spec-flow tooling in Python; schema-driven validation; V2 `.opencode/` layout
**Status:** accepted
**Date:** 2026-08-28

**Context.** Review of the Phase 1/2 tooling found four defects, three of them
real:

1. `check-opencode.ps1` understood only the **legacy singular** `.opencode/`
   layout (`agent/`, `command/`) while opencode's current — "V2" — layout is
   **plural** (`agents/`, `commands/`, `skills/`), with singular kept for
   backwards compatibility. Our own harness was inconsistent: plural `skills/`,
   singular `agent/` and `command/`.
2. Its frontmatter handling was hand-rolled regex. It could not report a YAML
   syntax error, only mis-parse one.
3. It asserted that undocumented permission keys were **INVALID**. The published
   schema declares `PermissionConfig.additionalProperties -> PermissionRuleConfig`,
   so such keys are *legal* — they are wildcard patterns matched against tool
   names, which is how MCP and custom tools are gated. The validator was stating
   a falsehood with confidence, which is the exact failure spec-flow exists to
   prevent.
4. `gate.ps1` set `$Skipped` after a failed `cmake --preset` and never read it,
   so a failed configure fell through into build, test and format, reporting
   results for a stale tree.

**Decision.**

- **Fail-fast in the gate.** A failed *configure* stops the run and still prints
  the `GATE SUMMARY` block, with downstream steps marked `SKIP` (exit `-1`,
  distinct from "ran and failed"). A failed *build* skips only the tests; the
  format check still runs because it is independent and cheap, so one invocation
  reports everything that is wrong.
- **Port the tooling to Python** (`scripts/gate.py`, `scripts/check_opencode.py`,
  `scripts/refresh_schema.py`); the PowerShell versions are deleted.
- **Validate against the real schema.** `scripts/refresh_schema.py` vendors
  `https://opencode.ai/config.json` to `scripts/schema/`, neutralising
  absolute-URL `$ref`s so validation is offline and deterministic, and recording
  every neutralised ref in an `x-vendored` block. `check_opencode.py` derives its
  accepted key sets from that schema instead of hardcoding lists that rot.
- **Real parsers, no regex.** PyYAML for frontmatter, `json` for config. A syntax
  error is now reported as the parser's own message. `scripts/requirements.txt`
  pins the two dependencies, and `check_opencode.py` **refuses to run** without
  them rather than silently degrading.
- **Adopt the V2 plural layout.** `.opencode/agents/`, `.opencode/commands/`,
  `.opencode/skills/`. The validator checks both generations and reports the
  singular form as deprecated.
- **Failure vs warning is now principled.** Only things that break or silently
  disable something fail. Legal-but-suspicious things warn, with the reason.

**Rationale.**

- PowerShell 7 is itself cross-platform; the language was never what made the
  gate Windows-only. The real constraint is MSVC discovery — `vswhere` plus
  `vcvarsall.bat` for `INCLUDE`/`LIB`, without which `cl.exe` will not run. That
  is equally platform-specific in any language. Python was chosen for the *other*
  reasons: one language for both scripts, and a real YAML/JSON-Schema ecosystem
  that removes the hand-rolled parsing entirely. Toolchain discovery is isolated
  in `find_toolchain()`, which does make the existing `linux-debug`/`macos-debug`
  presets reachable — though that path is **written and never executed** (T-033).
- Lua was rejected: no usable stdlib for subprocess/environment capture, no YAML,
  no ecosystem here. Strictly more code for strictly less capability.
- Vendoring the schema rather than fetching at runtime means the validator works
  offline, and any change to opencode's config surface arrives as a reviewable
  diff instead of silently altering what we enforce.

**Impact.**
- New: `scripts/gate.py`, `scripts/check_opencode.py`, `scripts/refresh_schema.py`,
  `scripts/requirements.txt`, `scripts/schema/opencode-config.schema.json`.
- Deleted: `scripts/gate.ps1`, `scripts/check-opencode.ps1`.
- Moved: `.opencode/agent/` → `agents/`, `.opencode/command/` → `commands/`.
- Every reference updated across `opencode.json`, the three skills, both agents,
  the three commands, `docs/`, and `.spec/README.md`.
- Tracker: T-036 done; **T-033, T-034, T-035** opened.

**Verification.** The new validator was tested against a fixture carrying planted
defects and reported all of them: legacy singular directory, skill name/directory
mismatch, non-slug directory name, malformed YAML, `prompt:` in frontmatter,
invalid `mode` enum, `steps: "lots"` type error, missing provider prefix, a
pattern map on an action-only permission, `template:` in a command, a nonexistent
`agent:`, a dangling `@agent` mention, and a dangling script path. It also caught
two real files whose `scripts/gate.ps1` references had been missed during the
rename — a defect found in this repository, not in a fixture.

**Consequences.** The tooling now needs Python 3.11+ plus two pip packages, in a
C++ repository that previously needed none; that cost is confined to `scripts/`
and the C++ build is untouched. The Linux/macOS gate path is declared but
unproven, so claiming cross-platform support would itself be an unverified claim
— it is tracked, not asserted.

## D-006 — spec-flow Phase 2: one SPEC per feature branch, author ≠ auditor enforced by permission
**Status:** accepted
**Date:** 2026-08-28

**Context.** D-005 adopted spec-flow and shipped Phase 1 (the gate and the
evidence format). Phase 2 is the other half: getting a written, audited
specification in front of the code instead of behind it.

**Decision.**

- **One SPEC per feature branch**, at `.spec/<branch-slug>/` where the slug is the
  branch name with `/` replaced by `-`. Both `spec.md` and `progress.md` are
  **committed**; `progress.md` is append-only.
- **EARS-lite requirements.** Five patterns (ubiquitous, event-driven,
  state-driven, unwanted, optional), every requirement carrying an ID and using
  SHALL. Every acceptance criterion names the command or doctest case that proves
  it; one that a machine cannot check must be marked `manual inspection` rather
  than disguised as automated.
- **Tiers S / M / L** decide depth. S skips the audit; L additionally requires
  *Alternatives considered*, *Rollback*, and a `D-NNN` entry.
- **Two read-only subagents**, both denied `edit` by permission rather than by
  instruction:
  - `@spec-auditor` (opus-5) grades a SPEC it did not write and returns
    `PASS` or `DEBT`. It may not rewrite what it grades.
  - `@verifier` (sonnet-5) runs `scripts/gate.py` itself and produces the
    five-section report. Its `bash` allowlist contains the gate and read-only
    git, nothing else.
- **Two commands.** `/plan` refuses to run on `main`/`master`, reads the code via
  `@explore` before writing, and re-audits at most twice. `/run` requires an
  audited SPEC, establishes a pre-change gate result, writes tests first, and
  iterates with the verifier at most three times.
- **Model routing** (T-026): opus-5 as the ceiling where judgment happens
  (`spec-auditor`, the built-in `plan` agent), sonnet-5 as the floor where work is
  mechanical (`verifier`, `explore`, `small_model`). The primary agent's model is
  deliberately **not** overridden — that stays the operator's choice.
- **No `implementer` subagent.** The primary agent writes the code. Adding a third
  subagent buys a context hop but no independence: independence comes from the
  verifier, which is separate regardless of who implemented.

**Rationale.**

- Per-branch directories mean two branches never collide and a merge never
  conflicts on the SPEC — the alternative, a single `.spec/spec.md`, conflicts on
  every second merge.
- Read-only auditors are a real boundary in opencode (`permission.edit: deny`),
  not a convention an agent can talk itself out of. This is the one part of
  MoAI-ADK's design that opencode enforces *better* than the original, which
  relies on prompt discipline.
- The round caps (2 audit, 3 verify) exist because repeated DEBT or repeated gate
  failure usually means the request or the approach is wrong. Grinding costs more
  than stopping and asking.

**Impact.**
- New: `.opencode/skills/spec-format/`, `.opencode/agent/{spec-auditor,verifier}.md`,
  `.opencode/command/{plan,run}.md`, `.spec/README.md`.
- New: `scripts/check_opencode.py` (T-032) — structural validation of the harness
  itself, because opencode fails *quietly* on a skill/directory name mismatch, an
  invalid `permission` key, or a missing `description`. Verified by planting seven
  defects in a throwaway agent file and confirming all seven were reported.
- `opencode.json` gains `small_model` and per-agent model routing.
- `docs/ai-instructions.md` rewritten to carry the four working rules and the
  repo's gate traps into every session (T-028).
- Tracker: T-026, T-027, T-028, T-032 done; **T-030, T-031** opened.
- No application source changes.

**Consequences.** Non-trivial work now starts with a document that a second model
has attacked. Cost rises by roughly two subagent round-trips per SPEC, paid at
the ceiling model for the audit. **The workflow is unexercised** — no SPEC has
been through `/plan` → `/run` yet (T-030), and the branch-slug path is derived in
prose, so a rename or detached HEAD breaks it silently (T-031). Whether the SPECs
prevent enough rework to pay for the round-trips remains unmeasured, by us and by
MoAI-ADK alike.

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
  `scripts/gate.py`, the `quality-gate` and `evidence-report` skills, the
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
  files only** (`--scope changed`), with `--scope branch` for PR time. This lets
  the gate be green today without pretending the debt does not exist (T-021).
- Two skills and one command is a small enough surface that a wrong call is
  cheap to notice; a 52-skill catalogue is not.

**Impact.**
- New: `scripts/gate.py`, `.opencode/`, `opencode.json`,
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
