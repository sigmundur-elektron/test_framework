---
branch: feat/permissions-enforce
tier: L
status: audited-with-debt
decision: D-008
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
document (`agent_export.cpp:83-88`) and disk persistence
(`src/data/persistence/ai_setup.h:25-33`).
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

R11. The system SHALL update the recorded gate baseline at
     `.opencode/skills/quality-gate/SKILL.md:56-62` to the counts this change
     produces. That baseline currently records `test cases: 22` and
     `may_fail assertions: 4`; A3 and A6 require both to move. `SKILL.md:83-85`
     instructs every agent to report a changed `may_fail` count as a finding, so
     leaving the baseline stale would make every subsequent gate run mismatch it
     by design and train readers to ignore the very signal that catches a
     deleted marker.

R12. The system SHALL mark **T-011** (`docs/status/tracker.md:27`) complete, that
     row being the task this SPEC discharges.

## Out of scope

- **Gating `agent_call_tool`.** No A2A scope exists in the `scope` enum, and
  adding one would violate R7 and break `permission_scope_id`
  (`agent_export.cpp:11-29`), `permission_scope_from_id`
  (`agent_template.cpp:16-26`) and the Glaze `enumerate` list
  (`src/data/persistence/ai_setup.h:25-33`).
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

**Verification is two-phase, and the order matters.** A1–A7 are evaluated on the
*uncommitted* working tree; A8–A12 read `origin/master...HEAD`, which sees
committed work only. Running them in the wrong order makes A7 vacuous: the
default gate's format step covers "files changed vs `HEAD` plus untracked"
(`.opencode/skills/quality-gate/SKILL.md:23`), so once the work is committed that
set is empty and the format check passes having examined nothing. The documented
alternative does not rescue it — `--scope branch` is described as covering
everything changed vs `origin/main` (`SKILL.md:24`), and this repo has no
`origin/main`; its base branch is `master`. So:

1. With the work staged but **not committed**, run `python scripts/gate.py` and
   evaluate A1–A7. Quote that output.
2. Commit, then evaluate A8–A12 against `origin/master...HEAD`.

Both outputs go in the evidence report. The `origin/main` reference at
`SKILL.md:24` is a pre-existing harness bug this SPEC works around rather than
fixes; it needs its own task.

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
    — verified by: `python scripts/gate.py`; that named case reported as passing
    without a `may_fail` waiver, and the summary field `may_fail assertions:`
    dropping from **4** to **3**. The named case is the criterion; the aggregate
    count alone would also drop if a different marker were deleted.

A4. The tool must be registered first: `read_file_tool` is registered by
    `agent::init()` (`agent.cpp:12`) into the process-wide `tool_registry`
    singleton, so each case calls `init()` before `execute_step`. Note that
    `test/read_file_tool_test.h:45` also registers a `read_file_tool` into that
    same singleton; `tool_registry.cpp:7` assigns by key, so the later
    registration overwrites rather than duplicates and the lookup is the same
    either way — but the registry is process-wide mutable state shared between
    test files, so no case may assume it is empty.
    An agent whose `grants` omit `read_project`, calling
    `agent::execute_step` with a step whose `tool_name` is the literal string
    `"read_file"` (`read_file_tool.cpp:5`; the registry keys on `tool->name()`
    at `tool_registry.cpp:7` and `agent.cpp:41` looks it up), receives
    `tool_result.success == false` and a message naming the denied scope —
    distinguishing it from the `"unknown tool: …"` path at `agent.cpp:43`, which
    names no scope — and the target file is not read. The last clause is observed
    as a proxy: the message does not contain the fixture's contents. Pick fixture
    content that cannot appear in a denial message (the `"hello agent"` used at
    `test/read_file_tool_test.h:29` is suitable). This proves the contents were
    not returned, not that the backend was never called; it is sound only because
    `read_file_tool.cpp:36` returns before reaching the read at `:40`. If that
    ordering changes, this criterion stops meaning what R6 asserts.
    — verified by: doctest case `[permissions] ungranted tool call is denied`
    in `test/permissions_test.h`

A5. An agent whose `grants` include `read_project`, calling
    `agent::execute_step` with the same `"read_file"` step, receives
    `tool_result.success == true` and output containing the file contents.
    — verified by: doctest case `[permissions] granted tool call succeeds`
    in `test/permissions_test.h`

A6. `test/permissions_test.h` is `#include`d in `src/main.cpp` alongside the
    existing test headers (T-022).
    — verified by: `python scripts/gate.py`; the doctest `test cases:` count rises
    from **22** to **26** — exactly the four cases named in A1, A2, A4 and A5

A7. The gate passes end to end: configure, build with **0 first-party warnings**,
    tests exit 0, `clang-format` clean on every file this change touches.
    — verified by: `python scripts/gate.py` reporting `GATE: PASS`, run in
    phase 1 above with the work **uncommitted**, so the format step has a
    non-empty file set. The pasted output must name the files it formatted; a
    format step reporting zero files is a failed A7, not a passed one.

A8. Export and persistence behaviour is unchanged: the existing cases in
    `test/agent_export_test.h`, `test/agent_template_test.h` and
    `test/ai_persistence_test.h` pass **without modification to those files**.
    — verified by: `python scripts/gate.py`; those three paths absent from
    `git diff --name-only origin/master...HEAD`. The base revision is required:
    bare `git diff --name-only` compares the working tree to the index, so it
    prints nothing once the work is staged and the check would pass vacuously.
    The base branch is `master`; there is no `main` in this repo.

A9. The four pre-existing direct `itool::execute` call sites named in R10 are
    updated to supply a `permissions` value, and their cases still pass.
    — verified by: `python scripts/gate.py`; `test/read_file_tool_test.h` and
    `test/agent_a2a_test.h` present in
    `git diff --name-only origin/master...HEAD` (same base revision as A8, and
    for the same reason — bare `git diff` would report absence for correct work
    as soon as it is staged), and the `test cases:` total consistent with A6
    (no case added or lost in either file)

A10. The recorded gate baseline at `.opencode/skills/quality-gate/SKILL.md:56-62`
     reads `test cases: 26` and `may_fail assertions: 3`, matching the numbers
     A6 and A3 require, and the assertion line is updated to the count the run
     actually reports. The `may_fail` note at `SKILL.md:80-85` no longer says
     "4 assertions fail by design".
     — verified by: `.opencode/skills/quality-gate/SKILL.md` present in
     `git diff --name-only origin/master...HEAD`, and the numbers in its
     baseline block equal to those in the `python scripts/gate.py` output
     pasted in the same evidence report. Because this edits a file under
     `.opencode/`, `python scripts/check_opencode.py` is also run and exits 0 —
     `docs/ai-instructions.md` requires it after any `.opencode/` change, and
     `SKILL.md:30-36` notes it is separate from the code gate, so `scripts/gate.py`
     does not cover it.

A11. The T-011 row no longer appears in the **Backlog** table
     (`docs/status/tracker.md:18-19` is its header; the row is at `:27`) and
     appears in the **Done** table (header at `:62-63`) with `Owner` set and
     `Updated` set to the merge date. The two tables have different columns —
     Backlog carries `Priority`, Done does not — so this is a row move and a
     reshape, not an edit in place. Bumping `Updated` on line 27 while leaving
     the row in Backlog does **not** satisfy this.
     — verified by: `git diff origin/master...HEAD -- docs/status/tracker.md`
     showing the row deleted from the Backlog table and added to the Done table

A12. The six `permissions::scope` enumerators keep their names, count and order,
     and `agent_config::grants` keeps its declared type — enforced at compile
     time, not by reading a diff. `test/permissions_test.h` contains, at file
     scope, one `static_assert` pinning each enumerator to its ordinal
     (`read_project` == 0 through `write_database` == 5, per `permissions.h:9-14`),
     a `static_assert` that `write_database` is the last by pinning the count,
     and a `static_assert(std::is_same_v<decltype(agent_config::grants),
     std::vector<permissions::scope>>)`.
     — verified by: `python scripts/gate.py`; the build step exits 0. Renaming,
     reordering, adding or removing an enumerator, or changing the `grants` type,
     fails compilation, which is what R7 and R8 assert. These are compile-time
     assertions and add no doctest case, so the A6 count of 26 is unaffected.

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
`agent_call_tool` gains a `permissions` parameter it does not use; under A7's
zero-warning threshold that parameter must be left unnamed or commented out, as
`read_file_tool.cpp:21` and `permissions.cpp:3` already do elsewhere.

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
`#include` from `src/main.cpp`. Restore the gate baseline at
`.opencode/skills/quality-gate/SKILL.md:56-62` to `test cases: 22` /
`may_fail assertions: 4` and its accompanying note, and reopen the T-011 row at
`docs/status/tracker.md:27` (R11, R12). **D-008 is not reverted** — `decisions.md:9`
declares the log append-only, so on rollback it is amended to `superseded` with a
pointer to why, rather than deleted. No data migration is involved: R7 and R8
keep the export and persistence formats byte-identical, so artifacts written
before or after the change remain readable either way.

## Risks

- **Serialization breakage.** The `scope` enum's names and order are load-bearing
  in three places — `agent_export.cpp:11-29`, `agent_template.cpp:16-26`,
  `src/data/persistence/ai_setup.h:25-33`. R7 forbids touching it; A12 enforces
  it at compile time and A8 checks the consequence.
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
- **The recorded gate baseline goes stale on merge.** This is the only change so
  far that moves `test cases:` and `may_fail assertions:`, the two numbers every
  evidence report is required to quote from
  `.opencode/skills/quality-gate/SKILL.md:56-62`. R11 and A10 exist to update
  them in the same commit. If they are skipped, every later agent reports a
  baseline mismatch as a finding, and the `may_fail` drop that `SKILL.md:83-85`
  is designed to flag — the signal that someone deleted a marker — becomes
  permanent noise.
