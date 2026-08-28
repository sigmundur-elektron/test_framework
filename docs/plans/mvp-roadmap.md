---
last-updated: 2025-02-14
owner: copilot
status: active
---

# MVP Roadmap

Goal: a user can **configure an AI agent setup for any project** — create
agents, author plans, coordinate them (A2A), and **export** the setup so an
external project runs it with an open coding agent.

Task IDs reference [`../status/tracker.md`](../status/tracker.md). Phases are
ordered but Phase 0 audits can run in parallel.

---

## Phase 0 — Audit & baseline (P0)
Confirm what actually exists before building on it. Resolves the open questions
in [`../notes/open-questions.md`](../notes/open-questions.md).

- **T-001** Agent layer audit — `planner`/`plan_step`/`agent_config`/`memory`.
- **T-002** Tool layer audit — registered tools + schema serializability.
- **T-003** Backends audit — completeness + MVP critical path.

**Exit criteria:** open questions Q1–Q10 answered; overviews updated to match
reality; export feasibility understood.

## Phase 1 — Agent configuration (P0)
The user can create and manage agents through the UI.

- **T-004** Verify agent CRUD end-to-end (`agent_panel` → `agent_service` →
  repository → registry): create, enable/disable, remove, peer-link.

**Exit criteria:** agents persist across restart; peer links settable.

## Phase 2 — Plan authoring (P0)
The user can turn a goal into an inspectable, runnable plan.

- **T-005** Preview + run a goal; render `plan_step`s in `agent_panel`.
- **T-011** Wire permissions from `agent_config` into tool execution (P1).

**Exit criteria:** preview shows ordered steps; run executes steps through the
tool layer honoring permissions.

## Phase 3 — Coordination / A2A (P0)
Agents coordinate with each other.

- **T-006** Exercise A2A delegation via `agent_call_tool` between two agents;
  confirm delegated results flow back.

**Exit criteria:** one agent can delegate a sub-goal to a peer and incorporate
the result.

## Phase 4 — Export (P0)
Serialize the agent setup into a portable artifact.

- **T-007** Define + freeze export schema v1 — see
  [`export-format.md`](export-format.md).
- **T-008** Implement export command (on `agent_service`) using Glaze JSON.
- **T-013** DocTest coverage for export (and A2A) paths (P1).

**Exit criteria:** a single command produces a portable file capturing agents,
configs, plans, tool bindings, and permissions.

## Phase 5 — Consume in a target project (P0)
An external project runs the exported setup with an open coding agent.

- **T-009** Document the consumer contract: how "open code" loads and runs the
  export (resolving Q11–Q13).
- **T-010** Optional import/round-trip back into the app (P2).

**Exit criteria:** a documented, demonstrated path where the export is loaded by
an external open coding agent and executes at least one plan.

---

## Out of scope for MVP / later
<!-- check-docs: allow-missing T-012 D-002 -->
- Cosmetic source-tree renames. A former entry here cited **T-012** and
  **D-002**; neither was ever written, and the rename it described named the
  same path as both source and target (`src/data/` → `src/data/`). Removed
  rather than back-filled with invented history. `scripts/check_docs.py` now
  fails on a reference to an ID that does not exist, which is how this was found
  — the suppression above is why naming them here does not re-trip it.
- Rich UI polish, multi-project management, remote sync of the repository.

## MVP definition of done
All P0 tasks (T-001..T-009) in **Done**; a demo runs the full loop:
create agents → author a plan → A2A coordinate → export → run externally.
