---
last-updated: 2025-02-15
owner: copilot
status: active
---

# Export Format (v1, frozen)

A **portable, self-describing artifact** that captures an agent setup so an
external project + open coding agent can run it. Serialized with **Glaze JSON**
(already used by `ai_persistence`). **Frozen as v1 under T-007** and implemented
under T-008 (`agent_service::export_setup` → `features::build_export_document`).

Related: [`mvp-roadmap.md`](mvp-roadmap.md) Phase 4/5,
open questions Q11–Q13 in [`../notes/open-questions.md`](../notes/open-questions.md).

## Design goals

- **Portable:** no dependency on the app's runtime pointers/registry.
- **Self-describing:** includes a schema `version`.
- **Complete:** agents, configs, plans, tool bindings, permissions.
- **Consumable by "open code":** tool bindings and permissions are explicit so
  an external agent knows what it may call and how.
- **Round-trip friendly (optional):** importable back into the app (T-010).

## Top-level shape

```jsonc
{
  "schema": "tf.agent-export",
  "version": 1,
  "exported_at": "2025-02-14T00:00:00Z",
  "source": { "app": "test_framework", "app_version": "0.1.0" },
  "agents": [ /* Agent[] */ ],
  "tools":  [ /* ToolBinding[] */ ],   // shared catalog referenced by agents
  "plans":  [ /* Plan[] */ ]            // authored plans, referenced by agents
}
```

## Agent

```jsonc
{
  "id": "researcher",
  "name": "Researcher",
  "endpoint": "https://...",           // from agent_config
  "enabled": true,
  "permissions": ["read_file", "project.read"],  // permission ids
  "peers": ["writer"],                 // A2A links (agent ids)
  "tools": ["read_file", "project_api.list"],    // tool ids it may use
  "plans": ["p-onboard"],              // plan ids owned by this agent
  "memory": { /* opaque, versioned memory snapshot */ }
}
```

## ToolBinding

Derived from each tool's `itool` `name` + `schema`. Lets the consumer validate
calls without the C++ tool present.

```jsonc
{
  "id": "read_file",
  "name": "read_file",
  "kind": "local | mcp | a2a",         // maps to tool layer categories
  "schema": { /* argument schema from itool.schema */ },
  "requires_permissions": ["read_file"],
  "mcp": { "transport": "..." }        // present only when kind == "mcp"
}
```

## Plan / PlanStep

Mirrors the agent-layer `plan_step` so plans re-run externally.

```jsonc
{
  "id": "p-onboard",
  "goal": "Summarize the repository",
  "steps": [
	{
	  "index": 0,
	  "tool": "read_file",             // ToolBinding id
	  "args": { "path": "README.md" },
	  "expects": "text"                // optional result contract
	}
  ]
}
```

## Permissions

Explicit list so the consumer can enforce authority independently of the app.

```jsonc
{ "id": "project.read", "description": "Read files/metadata from the project" }
```

## Open decisions (block freeze under T-007)

1. **Consumer target (Q11):** custom JSON (this proposal) vs. aligning to an
   existing agent spec or an MCP manifest. Decision needed before v1 freeze.
2. **Backends in export (Q10):** include backend endpoints/config, or leave the
   consuming "open code" agent to resolve them from its own environment?
3. **Memory portability (Q4):** ship full memory snapshot vs. a redacted/summary
   form; must be versioned.
4. **Round-trip (Q12/T-010):** confirm import symmetry and id-collision rules.
5. **Where export lives (Q13):** `agent_service` owns authoritative agents —
   proposed home for the export command.

## Resolved decisions (v1 freeze, 2025-02-15)

1. **Consumer target (Q11):** custom `tf.agent-export` JSON (this proposal).
2. **Backends in export (Q10):** **excluded** — no DB/backend endpoints in v1;
   the consuming "open code" agent resolves them from its own environment.
3. **Memory portability (Q4):** **full memory snapshot included**, carried in
   the versioned document (`version: 1`).
4. **Round-trip (Q12/T-010):** export is **one-way** for v1; import deferred.
5. **Where export lives (Q13):** **`agent_service::export_setup`**, delegating
   to `features::build_export_document` / `export_document_to_json`.

Implementation notes: `plans` is empty in v1 (no persisted authored plans yet);
tool bindings are derived from the live `tool_registry` (`kind: "local"`).

## Validation plan

- Add DocTest cases (T-013) that export a small setup and assert the JSON round-
  trips through Glaze and re-imports to an equivalent in-memory model.
