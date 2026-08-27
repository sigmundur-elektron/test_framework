---
last-updated: 2025-02-16
owner: copilot
status: active
---

# Open Questions

Only **currently open** items live here. This doc should trend toward *empty*.

**Lifecycle — when a question is resolved, remove it and route it:**
- Made a decision → record an ADR in [`decisions.md`](decisions.md) (reference
  the question id), then delete the question here.
- Spawned work → add a task in [`../status/tracker.md`](../status/tracker.md)
  (reference the question id), then delete the question here.
- No action / self-evident → just delete it.

Resolved history is preserved in `decisions.md` and `tracker.md`, not here.

---

- **Q2.** What exactly is in `plan_step` (tool binding, args, expected result)?
  Is it rich enough to export and re-run externally? Revisit once **T-005**
  gives the planner real output to shape `plan_step` against.
- **Q11.** What target format should the "open code" agent consume — custom
  `tf.agent-export` JSON (current v1 baseline, D-003), an existing agent spec,
  or an MCP manifest? Investigating before locking the consumer contract
  (**T-009**).
