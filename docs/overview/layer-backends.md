---
last-updated: 2025-02-14
owner: copilot
status: active
---

# Layer: Backends (Application APIs)

**Location:** `src/data/backends/`

The bottom layer. These are ordinary application APIs that tools call to do real
work. They contain no reasoning — they wrap concrete systems and return data or
perform side effects. Access is always mediated by the
[tool/workflow layer](layer-tools.md) (validation + permissions first).

## Modules

### `database_api` (`database_api.h`)
Application-level database operations. Distinct from the
[`repository/`](app-and-infra.md) layer (which persists *agents* themselves):
this API is a **backend a tool exposes to agents**.

### `github_api` (`github_api.h`)
GitHub operations (repos, issues, PRs, etc.) exposed to agents through a tool.

### `project_api` (`project_api.h`)
Operations against the target **project** — the surface an agent uses to inspect
or modify a project it has been assigned to.

## Responsibilities

- Wrap external/system APIs behind small, testable C++ interfaces.
- Return structured results tools can translate into `tool_result`.
- Stay reasoning-free; enforce nothing beyond their own API contract
  (permissions/validation are the tool layer's job).

## Current state (as understood)

- Three backend headers exist: `database_api.h`, `github_api.h`,
  `project_api.h`. Depth of implementation to be confirmed per module.

## MVP gaps / open items

- Identify which backend(s) are **required** for the MVP export/run flow —
  `project_api` is the most likely critical path (agents act on a project).
- Define minimal, stable data contracts each backend returns.
- Decide whether backends need to be part of the **export** (as tool bindings)
  or are resolved by the consuming "open code" agent — see
  [export-format](../plans/export-format.md).

See task IDs in [`../status/tracker.md`](../status/tracker.md).
