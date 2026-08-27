---
last-updated: 2025-02-14
owner: copilot
status: active
---

# Layer: Tool / Workflow

**Location:** `src/data/tools/`

The middle layer. It exposes a **uniform tool interface** the agent invokes,
validates arguments, enforces permissions, and orchestrates calls — including
external tools over MCP and delegation to peer agents (A2A). Tools call
**down** into the [backends](layer-backends.md).

## Modules

### `itool` (`itool.h`)
The uniform tool interface. Each tool provides:
- `name` — stable identifier.
- `schema` — argument schema (for validation and export).
- `execute(...)` — returns `std::expected<tool_result, std::string>`
  (success payload or error string).

Tools **validate arguments** and **check permissions** before calling a
backend.

### `tool_registry` (`tool_registry.h`)
Singleton catalog of available tools (mirrors the `input_manager` pattern).
Agents hold a reference to it and look tools up by name at execution time.

### `permissions` (`permissions.h`)
Permission checks gating tool execution. Intended to be driven by
`agent_config` so each agent's authority is explicit and exportable.

### `mcp/mcp_client.h`
MCP (Model Context Protocol) transport for **external** tools — lets the tool
layer reach tools/services outside the process.

### `agent_call_tool` (`agent_call_tool.h`) — A2A
Agent-to-agent delegation: one agent delegates a goal to a **peer** agent.
Enables coordination/orchestration across multiple agents.

### Concrete tools
- `read_file_tool` (`read_file_tool.h`) — example file-reading tool.
- Additional domain tools register into `tool_registry`.

## Responsibilities

- Present a single, validated execution contract (`itool`) to agents.
- Enforce permissions before any backend side effect.
- Orchestrate: local tools, MCP external tools, and A2A delegation.
- Provide machine-readable `schema` used for validation **and export**.

## Current state (as understood)

- `itool` contract with `std::expected` result is defined.
- `tool_registry` singleton exists; A2A via `agent_call_tool`; MCP client
  present; at least one concrete tool (`read_file_tool`).
- Permission checks are represented as a dedicated module.

## MVP gaps / open items

- Enumerate the **minimum tool set** required for the MVP export/run flow.
- Ensure every tool's `schema` is serializable for
  [export-format](../plans/export-format.md) (name, args, permission needs).
- Define permission model coupling with `agent_config` and export.
- Clarify MCP client transport readiness for external "open code" execution.

See task IDs in [`../status/tracker.md`](../status/tracker.md).
