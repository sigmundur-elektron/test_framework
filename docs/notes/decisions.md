---
last-updated: 2025-02-14
owner: copilot
status: active
---

# Decision Log (ADR-lite)

Append one entry per non-trivial decision. Newest at top.

---

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
