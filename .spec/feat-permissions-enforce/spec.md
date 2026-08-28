---
branch: feat/permissions-enforce
tier: L
status: draft
decision: D-008 (required before /run; not yet written)
last-updated: 2026-08-28
---

# SPEC — permissions::allowed honours the agent's granted scopes

## Context

`permissions::allowed` is a stub. `permissions.cpp:3-7` ignores its argument and
unconditionally returns `true`:

```cpp
bool permissions::allowed(scope /*s*/) const
{
	// TODO: replace with real policy. Default: read allowed, write gated.
	return true;
}
```

`permissions` (`permissions.h:5-18`) is a struct with **no member state** — there
is nowhere for a grant list to live. Meanwhile `agent_config::grants`
(`agent_config.h:12`) already stores `std::vector<permissions::scope>` per agent,
is already edited from the GUI, and already round-trips through both the export
document (`agent_export.cpp:83-88`) and disk persistence (`ai_setup.h:25-33`).
The data exists; nothing reads it when deciding anything.

The only call to `allowed()` in production code is `read_file_tool.cpp:36`, which
queries a default-constructed member `permissions _perms;` (`read_file_tool.h:14`)
that has no relationship to the agent invoking the tool. `itool::execute`
(`itool.h:27`) takes only `const std::string &json_args` — there is **no caller
or identity parameter of any kind**, so a tool cannot currently know whose grants
apply. `agent::execute_step` (`agent.cpp:39-50`) is the sole runtime tool-invocation
site and is the only place that holds both `_config` (hence `_config.grants`) and
the `tool->execute(...)` call.

Tracker task: **T-011** — "Permissions wired from `agent_config` → tool execution"
(Phase 2, P1). Historically labelled Q8; note that `docs/notes/open-questions.md`
no longer contains a Q8 entry, so the label survives only in code comments,
`docs/plans/mvp-roadmap.md:41` and the tracker.

`test/mvp_gaps_test.h:28-34` is the living-TODO marker for this gap and is one of
the four `may_fail` assertions in the recorded gate baseline.

## Requirements

R1. The system SHALL store, in each `permissions` instance, the set of scopes
    that instance grants.

R2. WHEN `allowed(s)` is called on a `permissions` instance whose grant set
    contains `s`, the system SHALL return `true`.

R3. WHEN `allowed(s)` is called on a `permissions` instance whose grant set does
    not contain `s`, the system SHALL return `false`.

R4. WHEN a default-constructed `permissions` instance is queried for any scope,
    the system SHALL return `false`.

R5. WHEN `agent::execute_step` invokes a tool, the system SHALL supply that tool
    with a `permissions` value constructed from the invoking agent's
    `agent_config::grants`.

R6. IF a tool requires a scope the invoking agent has not been granted, THEN the
    tool SHALL return `std::unexpected<std::string>` naming the denied scope and
    SHALL NOT perform the underlying backend operation, and
    `agent::execute_step` SHALL surface that as `tool_result{false, <message>}`
    (the mapping already present at `agent.cpp:46-47`).

R7. The system SHALL NOT change the names, count or declaration order of the
    `permissions::scope` enumerators.

R8. The system SHALL NOT change the declared type of `agent_config::grants`.

R9. `agent::execute_step` SHALL be invocable from a doctest case without
    constructing a plan. It is currently `private` (`agent.h:36`, under the
    `private:` label at `agent.h:30`) and the only public route to it is
    `agent::handle`, which dispatches steps from `planner::plan` — a stub that
    returns `{}` and is itself an open gap (T-005, `mvp_gaps_test.h:19-25`).
    Without this, R5 and R6 cannot be observed.

R10. Every existing doctest case that invokes `itool::execute` directly SHALL be
     updated to supply a `permissions` value and SHALL continue to pass. Today
     that is `test/read_file_tool_test.h:19`, `test/read_file_tool_test.h:33`,
     `test/agent_a2a_test.h:32` and `test/agent_a2a_test.h:36`.

## Out of scope

- **Gating `agent_call_tool`.** No A2A scope exists in the `scope` enum, and
  adding one would violate R7 and break `permission_scope_id`
  (`agent_export.cpp:11-29`), `permission_scope_from_id`
  (`agent_template.cpp:16-25`) and the Glaze `enumerate` list (`ai_setup.h:25-33`).
  Peer calls remain governed by `agent_config::peer_agents`. Needs its own task.
- **Populating `export_tool_binding::requires_permissions`** — declared at
  `agent_export.h:44`, never written by any code today. Stays empty.
- **Any change to the export schema or its version**, and any change to the
  persistence format.
- **GUI for editing grants.** `agent::mutable_config()` already exposes them.
- **MCP tools** — `mcp_client` is an empty stub (T-017).
- **A deny-list, wildcard or hierarchical policy.** Grants are a flat allow-list.

## Acceptance criteria

Every case below lives in `test/permissions_test.h` unless stated otherwise, and
every one is observed by running `python scripts/gate.py`.

A1. A `permissions` constructed from `{write_database}` returns `true` for
    `write_database` and `false` for the other five scopes.
    — verified by: doctest case `[permissions] explicit grant is honoured`
    in `test/permissions_test.h`

A2. A default-constructed `permissions` returns `false` for all six scopes.
    — verified by: doctest case `[permissions] default construction denies all`
    in `test/permissions_test.h`

A3. The existing case `[mvp-gap][permissions] denied scope is actually denied`
    (`test/mvp_gaps_test.h:28-34`) passes with its `* doctest::may_fail()`
    decorator **removed**.
    — verified by: `python scripts/gate.py`; the summary field
    `may_fail assertions:` drops from **4** to **3**

A4. An agent whose `grants` omit `read_project`, calling
    `agent::execute_step` with a step naming `read_file_tool`, receives
    `tool_result.success == false` and a message naming the denied scope, and the
    target file is not read.
    — verified by: doctest case `[permissions] ungranted tool call is denied`
    in `test/permissions_test.h`

A5. An agent whose `grants` include `read_project`, calling
    `agent::execute_step` with the same step, receives
    `tool_result.success == true` and output containing the file contents.
    — verified by: doctest case `[permissions] granted tool call succeeds`
    in `test/permissions_test.h`

A6. `test/permissions_test.h` is `#include`d in `src/main.cpp` alongside the
    existing test headers (T-022).
    — verified by: `python scripts/gate.py`; the doctest `test cases:` count rises
    from **22** to **26** — exactly the four cases named in A1, A2, A4 and A5

A7. The gate passes end to end: configure, build with **0 first-party warnings**,
    tests exit 0, `clang-format` clean on every file this change touches.
    — verified by: `python scripts/gate.py` reporting `GATE: PASS`

A8. Export and persistence behaviour is unchanged: the existing cases in
    `test/agent_export_test.h`, `test/agent_template_test.h` and
    `test/ai_persistence_test.h` pass **without modification to those files**.
    — verified by: `python scripts/gate.py`; those three paths absent from
    `git diff --name-only`

A9. The four pre-existing direct `itool::execute` call sites named in R10 are
    updated to supply a `permissions` value, and their cases still pass.
    — verified by: `python scripts/gate.py`; `test/read_file_tool_test.h` and
    `test/agent_a2a_test.h` present in `git diff --name-only`, and the
    `test cases:` total consistent with A6 (no case added or lost in either file)

## Alternatives considered

**A. Pass `permissions` through `itool::execute`, and expose `execute_step` (chosen).**
Change `itool.h:27` to
`execute(const std::string &json_args, const permissions &perms)`, drop the
`permissions _perms` member from `read_file_tool.h:14`, have
`agent::execute_step` construct `permissions{_config.grants}` and pass it, and
move `execute_step` from `private` to the public section of `agent` so R5/R6 are
observable (R9). Every tool then gates itself against the *caller's* grants,
matching the comment already at `itool.h:26` ("Validate → check permissions →
call backend").
Full cost — every file the signature change touches:
`itool.h`, `read_file_tool.h/.cpp`, `agent_call_tool.h/.cpp`, `agent.h`,
`agent.cpp`, and **four test call sites in two files**:
`test/read_file_tool_test.h:19,33` and `test/agent_a2a_test.h:32,36`.

**B. Gate centrally in `agent::execute_step`.**
Keep `itool::execute` unchanged; have `execute_step` consult a
tool-name → required-scope table before dispatching. Rejected: the mapping would
live outside the tool that knows its own requirements, drifting silently as tools
are added, and `itool` gains no way to express what it needs. It also leaves the
inert `_perms` member in `read_file_tool` as a trap. It would, however, avoid
touching any test file — a real advantage that did not outweigh the drift risk.

**C. Give `permissions` state but leave the wiring alone.**
Smallest diff, and it would make the `may_fail` marker pass. Rejected as
actively harmful: `read_file_tool`'s default-constructed `_perms` would begin
denying `read_project`, breaking `test/read_file_tool_test.h:33`, while removing
the visible marker that says the gap is open. It would trade a known gap for a
hidden one.

**D. Add an `agent`/caller-context parameter instead of `permissions`.**
Rejected for this change: it couples the tool layer to the agent layer, inverting
the dependency direction in `docs/ai-instructions.md`. Passing only the resolved
grant set keeps tools ignorant of agents.

**E. Reach `execute_step` via `agent::handle` instead of changing visibility.**
Rejected: `handle` dispatches steps produced by `planner::plan`, which returns
`{}` and is an open gap (T-005). Depending on it would make this SPEC's
acceptance untestable until an unrelated task lands.

## Rollback

The change is additive to `permissions`, one signature change, and one visibility
change. To revert: restore `permissions.cpp` to `return true`, drop the grant
member and constructor from `permissions.h`, restore `execute(const std::string&)`
on `itool` and both implementers, restore `permissions _perms` in
`read_file_tool.h`, move `agent::execute_step` back under `private:`, revert the
four test call sites named in R10, reinstate `* doctest::may_fail()` on the
`[mvp-gap][permissions]` case, and remove `test/permissions_test.h` and its
`#include` from `src/main.cpp`. No data migration is involved: R7 and R8 keep the
export and persistence formats byte-identical, so artifacts written before or
after the change remain readable either way.

## Risks

- **Serialization breakage.** The `scope` enum's names and order are load-bearing
  in three places — `agent_export.cpp:11-29`, `agent_template.cpp:16-25`,
  `ai_setup.h:25-33`. R7 forbids touching it; A8 checks the consequence.
- **Deny-by-default is a behaviour change.** Any agent whose `grants` are empty
  loses tool access it silently had. Existing persisted agents and any test
  constructing a bare `agent_config` are affected. Blast radius is every tool
  call routed through `agent::execute_step`.
- **Interface change ripples.** Every current and future `itool` implementer must
  take the new parameter, and so must every direct caller. The four known test
  call sites are enumerated in R10; a fifth appearing between now and `/run`
  would not be caught by anything but the compiler.
- **Widening `agent`'s public surface.** Making `execute_step` public to satisfy
  R9 exposes a method that was deliberately private. It is a test seam as much as
  an API, and nothing prevents UI code from calling it directly thereafter.
- **Silent test omission.** If `test/permissions_test.h` is not added to
  `src/main.cpp`, the new cases never run and nothing reports it (T-022). A6
  exists specifically to catch that via the test-count delta.
- **`read_file_tool` reads the real filesystem.** New tests must create and clean
  up temp files, following the `std::ofstream` pattern already used in
  `test/read_file_tool_test.h`.
