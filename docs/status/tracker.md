---
last-updated: 2026-08-28
owner: copilot
status: active
---

# Task Tracker

Single source of truth for work state. Move tasks between sections and keep
`Owner` / `Updated` current. Task IDs (`T-NNN`) are referenced from
[`../plans/mvp-roadmap.md`](../plans/mvp-roadmap.md).

**Legend:** Priority = P0 (critical) · P1 (high) · P2 (nice-to-have).
Phase maps to the [MVP roadmap](../plans/mvp-roadmap.md).

## Backlog

| ID | Task | Phase | Priority | Owner | Updated |
|----|------|-------|----------|-------|---------|
| T-002 | Audit tool layer: enumerate registered tools + schema serializability (Q5–Q8) | 0 | P0 | — | 2025-02-14 |
| T-003 | Audit backends: completeness + MVP critical path (Q9–Q10) | 0 | P1 | — | 2025-02-14 |
| T-004 | Agent CRUD in UI via `agent_service` (create/enable/remove/peer-link) verified end-to-end | 1 | P0 | — | 2025-02-14 |
| T-005 | Plan authoring: preview + run a goal, surface `plan_step`s in `agent_panel` | 2 | P0 | — | 2025-02-14 |
| T-006 | Coordination: exercise A2A via `agent_call_tool` between two agents | 3 | P0 | — | 2025-02-14 |
| T-009 | Consumer contract: document how an "open code" agent runs the export (Q11–Q13) | 5 | P0 | — | 2025-02-14 |
| T-010 | Import/round-trip support (two-way export — promoted to MVP per D-004, Q12) | 5 | P0 | — | 2025-02-16 |
| T-011 | Permissions wired from `agent_config` → tool execution | 2 | P1 | — | 2025-02-14 |
| T-015 | Define + implement MVP tool set: agent CRUD tools + git/gitlab tool (Q5) | 2 | P0 | — | 2025-02-16 |
| T-016 | Make `itool::schema` a serializable struct instead of a raw string (Q6) | 2 | P1 | — | 2025-02-16 |
| T-017 | Implement `mcp_client` transport (handshake + tool discovery) (Q7) | 2 | P0 | — | 2025-02-16 |
| T-018 | CI/CD pipeline (planned, not implemented) (Q14) | 5 | P2 | — | 2025-02-16 |
| T-020 | Measure clean-build warning count; make "0 new first-party warnings" a real gate threshold | 0 | P1 | — | 2026-08-28 |
| T-021 | Format debt: 44 of 80 tracked `src/`+`test/` files fail `clang-format`. Decide bulk-reformat vs. leave, then widen gate `-Scope` to `All` | 0 | P1 | — | 2026-08-28 |
| T-022 | Test files are `#include`d by hand in `src/main.cpp`; a new `test/*_test.h` silently never runs. Auto-discover or assert an expected test-case count | 0 | P1 | — | 2026-08-28 |
| T-023 | No static analysis: add `.clang-tidy` + `CMAKE_EXPORT_COMPILE_COMMANDS`, then add a gate step | 0 | P2 | — | 2026-08-28 |
| T-024 | No coverage measurement. Decide whether coverage is a gate threshold at all on MSVC | 0 | P2 | — | 2026-08-28 |
| T-025 | Gate is advisory only — nothing blocks a commit that skipped it. Add a git `pre-commit` hook running `scripts/gate.py` | 0 | P1 | — | 2026-08-28 |
| T-029 | `ctest` is not wired (`enable_testing()`/`add_test()` absent, no build/test presets). Decide whether to wire it or document the app-binary invocation as permanent | 0 | P2 | — | 2026-08-28 |
| T-030 | spec-flow Phase 2 is unexercised: no SPEC has been run through `/plan` → `/run` yet. Validate on the first real feature branch | 0 | P1 | — | 2026-08-28 |
| T-031 | `/plan` and `/run` derive the SPEC path from the branch name in prose; a rename or detached HEAD breaks it silently. Consider a `scripts/spec-path.ps1` helper | 0 | P2 | — | 2026-08-28 |
| T-033 | `gate.py` Linux/macOS toolchain path is written but **never executed**. The `linux-debug`/`macos-debug` presets are unverified | 0 | P2 | — | 2026-08-28 |
| T-034 | `scripts/requirements.txt` (pyyaml, jsonschema) is installed manually. No pinning, no venv, no CI step to install it | 0 | P2 | — | 2026-08-28 |
| T-035 | Vendored `scripts/schema/opencode-config.schema.json` can drift from upstream. Nothing detects staleness; re-run `scripts/refresh_schema.py` periodically | 0 | P2 | — | 2026-08-28 |
| T-037 | `/plan`'s two-round audit cap fired on an unambiguous request. Its rationale assumes repeated DEBT means an unclear ask; it also catches "author keeps getting facts wrong", which needs a different response. Distinguish the two, or raise the cap for factual defects | 0 | P2 | — | 2026-08-28 |
| T-038 | `@spec-auditor` round 2 left most Context/Out-of-scope `file:line` citations unverified (its own Gaps section). Consider requiring the auditor to re-verify citations it checked in an earlier round | 0 | P2 | — | 2026-08-28 |

## In Progress

| ID | Task | Phase | Priority | Owner | Updated |
|----|------|-------|----------|-------|---------|
| T-014 | "Save as template" for agents + de-emphasize export UI (D-004) | 1 | P0 | copilot | 2025-02-16 |

## Blocked

| ID | Task | Blocked by | Owner | Updated |
|----|------|-----------|-------|---------|
| — | — | — | — | — |

## Done

| ID | Task | Phase | Owner | Updated |
|----|------|-------|-------|---------|
| T-000 | Establish `docs/` coordination workspace (overviews, notes, tracker, plans) | 0 | copilot | 2025-02-14 |
| T-001 | Audit agent layer (answers Q1–Q4: planner is a stub, config fields confirmed, memory covered by test) | 0 | copilot | 2025-02-16 |
| T-007 | Freeze export schema v1 (`tf.agent-export`); see `plans/export-format.md` | 4 | copilot | 2025-02-15 |
| T-008 | Implement export command `agent_service::export_setup` (Glaze JSON) | 4 | copilot | 2025-02-15 |
| T-013 | DocTest coverage for export (`test/agent_export_test.h`) | 4 | copilot | 2025-02-15 |
| T-019 | spec-flow Phase 1: `scripts/gate.py`, `quality-gate` + `evidence-report` skills, `/gate` command, `opencode.json` | 0 | opencode | 2026-08-28 |
| T-026 | spec-flow model routing wired: opus-5 ceiling (`spec-auditor`, `plan`), sonnet-5 floor (`verifier`, `explore`, `small_model`) | 0 | opencode | 2026-08-28 |
| T-027 | spec-flow Phase 2: `spec-format` skill, `.spec/<branch-slug>/`, `/plan` + `/run`, `spec-auditor` + `verifier` agents (D-006) | 0 | opencode | 2026-08-28 |
| T-028 | Working rules restated in `docs/ai-instructions.md` so they load in every session | 0 | opencode | 2026-08-28 |
| T-032 | `scripts/check_opencode.py` — structural validation of `.opencode/` (skill name/dir match, valid permission keys, required descriptions); verified against 7 planted defects | 0 | opencode | 2026-08-28 |
| T-036 | Port `gate.ps1`/`check-opencode.ps1` to Python; schema-driven validation, real YAML parsing, fail-fast on configure, V2 plural `.opencode/` layout (D-007) | 0 | opencode | 2026-08-28 |
