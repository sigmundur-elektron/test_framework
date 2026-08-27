---
last-updated: 14-02-2025
owner: copilot
status: active
---

# AI Coordination Workspace

This folder is the **shared coordination workspace for AI agents** (GitHub
Copilot and any peer/automation agents) working on the `test_framework`
project. It is **not** application source code and is intentionally kept out of
`src/` so it is never swept into the CMake build
(`file(GLOB_RECURSE src/*.cpp)`).

## How to use this folder

1. **Read first:** [`overview/project-overview.md`](overview/project-overview.md)
   for the big picture, then the per-layer overviews.
2. **Before doing work:** check [`status/tracker.md`](status/tracker.md) for the
   current task board and pick/claim a task by its ID (e.g. `T-003`).
3. **When you make a non-trivial choice:** append an entry to
   [`notes/decisions.md`](notes/decisions.md).
4. **When state changes:** update [`status/tracker.md`](status/tracker.md)
   (move the task, set `owner`, bump `last-updated`).
5. **When you hit an unknown:** record it in
   [`notes/open-questions.md`](notes/open-questions.md).

## File map

| Path | Purpose |
|------|---------|
| [`overview/project-overview.md`](overview/project-overview.md) | Goals, MVP definition, architecture, build/test/run, glossary |
| [`overview/layer-agent.md`](overview/layer-agent.md) | Reasoning layer: agent, planner, memory, config, registry |
| [`overview/layer-tools.md`](overview/layer-tools.md) | Tool/workflow layer: itool, registry, permissions, MCP, A2A |
| [`overview/layer-backends.md`](overview/layer-backends.md) | Backends: database, GitHub, project APIs |
| [`overview/app-and-infra.md`](overview/app-and-infra.md) | features service, repository, UI, build/test infra |
| [`notes/decisions.md`](notes/decisions.md) | Decision log (ADR-lite) |
| [`notes/open-questions.md`](notes/open-questions.md) | Unresolved questions |
| [`status/tracker.md`](status/tracker.md) | Task board — single source of truth |
| [`plans/mvp-roadmap.md`](plans/mvp-roadmap.md) | Phased path to MVP |
| [`plans/export-format.md`](plans/export-format.md) | Portable export schema proposal |

## Conventions

- **Front-matter:** every artifact begins with a YAML block:
  ```yaml
  ---
  last-updated: DD-MM-YYYY
  owner: <agent-or-person>
  status: draft | active | superseded
  ---
  ```
- **Task IDs:** tasks in the tracker use `T-NNN` and are referenced from plans
  for traceability.
- **Authority:** the project [`README.md`](../README.md) is authoritative for
  build and profiling facts; if this workspace drifts, reconcile toward it.
- **No code here:** this folder holds documentation only. Code changes belong in
  `src/` and must be tracked as their own task.

## Doc lifecycle (keep docs lean)

Docs are separated by lifecycle so they scale over time:

- **`notes/open-questions.md`** — only *currently open* items; trends toward
  empty. When a question is resolved, **remove it** and route it: a decision →
  `notes/decisions.md` (ADR), spawned work → `status/tracker.md` (task),
  self-evident/no-action → just delete. Reference the question id from the ADR
  or task for traceability.
- **`notes/decisions.md`** — append-only history of *why*; meant to grow.
- **`status/tracker.md`** — actionable follow-ups (`T-NNN`); tasks move to Done.
- **`overview/`** — describe *current reality*; rewrite in place, don't
  accumulate historical narrative.
- **`plans/`** — completed phases collapse to a one-line "Done — see tracker".
