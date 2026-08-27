---
last-updated: 2025-02-16
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
