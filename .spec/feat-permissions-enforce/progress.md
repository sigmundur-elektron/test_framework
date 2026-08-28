# progress — feat/permissions-enforce

## 2026-08-28 — plan

Request (verbatim):

> permissions::allowed should honour the agent's granted scopes

Branch `feat/permissions-enforce`, clean tree at `d280815`. No prior SPEC in
`.spec/feat-permissions-enforce/`.

Code read before writing (via @explore), not from memory:

- `permissions.h:5-18` — struct, no member state, six-enumerator `scope` enum.
- `permissions.cpp:3-7` — `allowed()` ignores its argument, returns `true`.
- `agent_config.h:12` — `std::vector<permissions::scope> grants;` already exists.
- `read_file_tool.h:14` / `read_file_tool.cpp:36` — the only production call to
  `allowed()`, against a default-constructed member with no link to any agent.
- `itool.h:27` — `execute(const std::string &json_args)` takes **no caller
  context**; there is no channel for grants to reach a tool.
- `agent.cpp:39-50` — `agent::execute_step` is the sole tool-invocation site and
  the only place holding both `_config.grants` and the `tool->execute` call.
- `mvp_gaps_test.h:28-34` — the `may_fail` marker for this gap.
- `agent_export.cpp:11-29`, `agent_template.cpp:16-25`, `ai_setup.h:25-33` —
  three independent tables keyed on the `scope` enum's names and order.
- `main.cpp:2-8` — the hand-maintained test `#include` block (T-022).

Tier assigned **L**, not M. Two reasons, both from the code rather than taste:
the fix cannot be made without deciding how grants reach a tool, and
`itool::execute` has no parameter for it — so a public interface changes and the
work spans the tools and agent layers. Per `spec-format`, that is L, which also
requires *Alternatives considered* and *Rollback* sections (both written) and a
`D-NNN` entry in `docs/notes/decisions.md` before `/run`.

Noted while writing: the minimal-diff approach (give `permissions` state, change
nothing else) would make the `may_fail` marker pass while simultaneously breaking
`read_file_tool_test.h`, because `read_file_tool`'s default-constructed `_perms`
would start denying `read_project`. That would trade a visible gap for a hidden
one. Recorded as rejected alternative C.

SPEC written: 9 requirements, 8 acceptance criteria, 4 alternatives, 5 risks.
Status: draft — awaiting audit.

### Auditor verdict — round 1: DEBT

Five blocking defects, all valid. Two rested on facts I checked myself rather
than accepting:

- `agent.h:36` — `execute_step` is indeed under the `private:` label at
  `agent.h:30`. **A4/A5 as written were untestable.** The only public route is
  `agent::handle`, which dispatches from `planner::plan` — a stub returning `{}`
  (T-005). Confirmed.
- `test/agent_a2a_test.h:32` and `:36` — two further direct `tool.execute(...)`
  call sites I had not listed. Confirmed by grep; four exist in total, not two.

Defects and fixes:

1. **A4/A5 unverifiable (private `execute_step`).** Added **R9** requiring the
   method be invocable from a doctest case, chose visibility change in
   Alternative A, added Alternative E recording why routing through `handle` was
   rejected, and added the widened public surface to Risks.
2. **R9 was filler** — no criterion, no such call site, no enforcement mechanism
   named. **Removed.** The signature change already forces callers to supply a
   value; a requirement restating a compiler guarantee earns nothing.
3. **Missed ripple: `test/agent_a2a_test.h`.** Alternative A's cost list now
   enumerates all four call sites across both files.
4. **`read_file_tool_test.h` edit existed only as prose.** Promoted to **R10**
   with **A9** behind it, and A8 reworded to say "without modification to those
   files" so the two sets are explicit rather than merely non-contradictory.
5. **Tier L has no `D-NNN`.** Frontmatter now names **D-008**, marked as required
   before `/run` and not yet written.

Nits also taken: A2/A4/A5 now name their file and the run command consistently
with A1; R6 states the concrete error shape (`std::unexpected<std::string>` from
the tool, mapped to `tool_result{false, …}` at `agent.cpp:46-47`).

Requirements 9 -> 10, acceptance criteria 8 -> 9, alternatives 4 -> 5.
Re-audit follows (round 2 of a maximum of 2).

### Auditor verdict — round 2: DEBT (1 blocking defect)

All five round-1 defects confirmed fixed, and the fixes judged sound: the
visibility change does make A4/A5 reachable (`agent(agent_config)` is public,
`plan_step` is a two-string aggregate), the four R10 call sites and their line
numbers are correct, D-008 is a free ID, and the A3 (4->3) and A6 (22->26)
arithmetic both hold. Every requirement R1-R10 now has a criterion behind it.

Remaining defect:

- **A4/A5 name the wrong tool.** They specify "a step naming `read_file_tool`",
  but the registry key is the tool's `name()` string — `"read_file"`
  (`read_file_tool.cpp:5`, keyed at `tool_registry.cpp:7`, looked up at
  `agent.cpp:41`). Verified directly, not taken on the auditor's word. Taken
  literally, A5 fails outright and **A4 passes for the wrong reason**: it would
  see `success == false` with the message `"unknown tool: read_file_tool"`, which
  names no scope and proves nothing about permissions.

**Stopped at the two-round cap with DEBT outstanding, per `/plan` step 6.**
Status remains `draft`. Not amended further, and not waived: the rule exists so
the author cannot decide its own work is close enough.

Note for the harness, not for this SPEC: the cap fired correctly but its stated
rationale did not apply. It assumes repeated DEBT means an unclear request; here
the request was unambiguous and the repeats were two independent factual errors
by the author. The cap conflates "the ask is unclear" with "the author keeps
getting facts wrong", which want different responses. Raised as T-037.
