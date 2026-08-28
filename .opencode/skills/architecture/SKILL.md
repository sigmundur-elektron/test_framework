---
name: architecture
description: Use when adding or modifying a tool, backend, agent, UI panel, service or repository in test_framework, when deciding which directory new code belongs in, or when tracing how a request flows from the UI down to a backend. Describes the actual layering under src/, the dependency direction rule, and the seams that are currently stubs.
---

# Architecture — test_framework

A desktop agent framework: a GLFW/Dear ImGui shell around an agent runtime that
dispatches tools. The graphics layer is the *shell*, not the product.

## Layers

```
UI            src/ui/          panels, window, theming  (ImGui, immediate mode)
Features      src/features/    services: orchestration + use cases
Agent         src/data/agent/  agent, planner, memory, config, registry
Tools         src/data/tools/  itool, tool_registry, permissions, mcp/
Backends      src/data/backends/   database_api, github_api, project_api
Persistence   src/data/persistence/  ai_persistence, ai_setup (Glaze JSON)
Repository    src/repository/  i_repository + in-memory / postgres
Utils         src/utils/       profiler, console, singleton, input
```

## The dependency direction rule

**Tools must not know about agents.** `itool::execute` takes the caller's
already-resolved `permissions`, not an `agent` or a context object
(`src/data/tools/itool.h:33-34`). This is deliberate and was decided in D-008:
passing an agent would invert the layering and couple the tool layer upward.

Concretely, when adding a capability:

- A tool receives **data it can act on**, plus the caller's grants. Nothing else.
- The agent layer resolves grants from `agent_config::grants` at dispatch time.
- Backends are called *by* tools, never directly by the UI.

## Where new code goes

| You are adding | Put it in | Also do |
|---|---|---|
| A new capability an agent can invoke | `src/data/tools/<name>_tool.{h,cpp}` | implement `itool`, register it, gate it on a `permissions::scope` |
| A new external system | `src/data/backends/<name>_api.{h,cpp}` | call it from a tool, not from UI |
| Orchestration across layers | `src/features/<name>_service.{h,cpp}` | keep the UI thin |
| A screen or widget | `src/ui/<name>_panel.{h,cpp}` | immediate-mode; no retained state in the panel |
| Storage | `src/repository/` behind `i_repository.h` | |

## Adding a tool — the checklist

1. Implement `itool`: `name()`, `schema()`, `execute(json_args, perms)`.
   Return `std::expected<tool_result, std::string>`.
2. **Gate it.** First thing in `execute`, check
   `perms.allowed(permissions::scope::<x>)` and fail closed. Default-constructed
   `permissions` grants nothing (`permissions.h:18-20`), so a tool reached
   without a resolved grant set denies rather than allows.
3. **Register it** in the process-wide `tool_registry`. The registry key is the
   string returned by `name()` — e.g. the literal `"read_file"`, *not*
   `read_file_tool`. An acceptance criterion naming the class instead of the key
   is a criterion that passes for the wrong reason; this exact mistake was
   caught in a `feat/permissions-enforce` audit round.
4. Add a `test/<name>_tool_test.h` — and load the `add-a-test` skill, because the
   include in `src/main.cpp` is manual and nothing catches its absence (T-022).

## Permissions

`permissions::scope` (`src/data/tools/permissions.h:8-16`) has six enumerators:
`read_project`, `write_project`, `read_github`, `write_github`, `read_database`,
`write_database`.

**The enum is frozen.** Its names, order and count are load-bearing in three
tables that are never compiled against each other:

- `permission_scope_id` — `src/features/agent_export.cpp`
- `permission_scope_from_id` — `src/features/agent_template.cpp`
- the Glaze `enumerate` list — `src/data/persistence/ai_setup.h`

A rename, reorder, insertion or removal silently changes what previously
exported documents mean. `test/permissions_test.h` pins every ordinal with
`static_assert`. **Appending a seventh enumerator is not caught** — C++23 cannot
assert enum cardinality without reflection (T-042). If you add a scope, update
all three tables by hand and say so.

## Stubs — do not mistake these for working code

These are known gaps with `may_fail` markers in `test/mvp_gaps_test.h`:

| Component | State | Task |
|---|---|---|
| `planner::plan` | returns `{}` — no steps ever | T-005 |
| `mcp_client` | `connect` / `register_discovered_tools` are empty | T-017 |
| `github_api` | returns no issues | T-003 |

`agent::handle` dispatches through `planner::plan`, so it is effectively inert.
**`agent::execute_step` is the live seam** and is public specifically so it is
testable (D-008) — drive tools through it, not through `handle`.

## A2A

Agent-to-agent calls go through `agent_call_tool` and are governed by
`agent_config::peer_agents`. They are **deliberately ungated**: no A2A scope
exists, and adding one would break the three frozen tables above (D-008).
