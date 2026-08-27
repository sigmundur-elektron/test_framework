---
last-updated: 2025-02-14
owner: copilot
status: active
---

# Layer: Agent (Reasoning)

**Location:** `src/data/agent/` (candidate rename → `src/data/`, see
[decisions](../notes/decisions.md))

The reasoning layer. It owns the *what to do and how to remember it*: agent
lifecycle, planning/task decomposition, memory, and configuration. It calls
**down** into the [tool/workflow layer](layer-tools.md) to actually perform
actions.

## Modules

### `agent` (`agent.h` / `agent.cpp`)
One configurable reasoning unit; multiple agents can coexist at runtime.

- Constructed from an `agent_config`.
- Lifecycle: `init()` → `handle(user_goal)` (loop) → `end()`.
- Holds a **reference** to the shared `tool_registry` (`tool_registry& _tools`),
  a `planner`, and `memory`.
- `preview(user_goal)` returns a non-mutating `std::vector<plan_step>` for the
  planner UI (does not touch memory).
- Exposes `config()` / `mutable_config()` (GUI edits live) and
  `get_memory()` / `mutable_memory()` (for save/load).
- Private `execute_step(plan_step)` runs a single step and returns a
  `tool_result`.

### `planner` (`planner.h`)
Task decomposition / reasoning. Turns a `user_goal` into an ordered
`std::vector<plan_step>`. Used both for preview (read-only) and execution.

### `memory` (`memory.h`)
Context + short/long-term memory for an agent. Serialized via `ai_persistence`
(Glaze JSON).

### `agent_config` (`agent_config.h`)
Configuration payload for an agent (e.g. name, endpoint, enabled state, peer
links, permissions). Edited live from the GUI via `mutable_config()`.

### `agent_registry`
Runtime, in-RAM catalog (singleton) of all live agents. Kept **in sync** with
the durable repository by [`agent_service`](app-and-infra.md). The UI never
touches the registry directly.

## Responsibilities

- Represent an agent and its lifecycle.
- Decompose goals into ordered, previewable steps.
- Maintain per-agent memory that survives restarts (via persistence).
- Expose config for live editing and durable storage.

## Current state (as understood)

- Core types exist: `agent`, `planner`, `memory`, `agent_config`, and an
  `agent_registry` singleton.
- `agent` orchestrates config + planner + memory + tools; `preview`/`handle`
  entry points are present.
- Persistence via `ai_persistence` (Glaze JSON) is referenced in README.

## MVP gaps / open items

- Confirm planner produces meaningful multi-step decompositions (not stubs).
- Confirm `plan_step` schema is rich enough to **export** (tool binding, args,
  expected result) — feeds [export-format](../plans/export-format.md).
- Memory schema needs a stable, versioned serialization for export/import.
- Define how config-level **permissions** flow into tool execution.

See task IDs in [`../status/tracker.md`](../status/tracker.md).
