---
name: spec-format
description: Use when authoring, auditing or amending a SPEC under .spec/, and when starting work on a feature branch that needs one. Defines the EARS-lite requirement syntax, the acceptance-criteria rule, the S/M/L tiers, and the progress-log format for test_framework.
---

# SPEC format — test_framework

One SPEC per feature branch. It is the source of truth for that branch's work.
If the code and the SPEC disagree, one of them is a bug — say which.

## Where it lives

```
.spec/<branch-slug>/
    spec.md        the specification
    progress.md    append-only log of what ran and what it printed
```

`<branch-slug>` is the current git branch with `/` replaced by `-`:

```powershell
(git rev-parse --abbrev-ref HEAD) -replace '/','-'
# feat/tls-timeout  ->  .spec/feat-tls-timeout/
```

Per-branch directories mean two branches never collide, and merging a branch
into `main` never conflicts on the SPEC. Both files are **committed** — the
audit trail is the point.

`.spec/` sits at the repo root, outside `src/`, so `file(GLOB_RECURSE src/*.cpp)`
never sweeps it into the build.

## Template

```markdown
---
branch: feat/tls-timeout
tier: M
status: draft
last-updated: 2026-08-28
---

# SPEC — handshake timeout

## Context
Two or three sentences: why this work exists, and what is true today.
Reference tracker IDs (`T-NNN`) and decisions (`D-NNN`) where they apply.

## Requirements
R1. WHEN <trigger>, the system SHALL <response>.
R2. WHILE <state>, the system SHALL <response>.
R3. IF <condition>, THEN the system SHALL <response>.
R4. WHERE <feature is included>, the system SHALL <response>.
R5. The system SHALL <invariant>.

## Out of scope
Explicit non-goals. This section prevents scope creep more than any other.

## Acceptance criteria
A1. <observable outcome>  — verified by: <exact command or test name>
A2. <observable outcome>  — verified by: <exact command or test name>

## Risks
What could go wrong and the blast radius.
```

`status` moves `draft` → `audited` → `in-progress` → `done`.
For **Tier L** add an `## Alternatives considered` section and a
`## Rollback` section.

## The EARS-lite patterns

| Pattern | Shape | Use for |
|---|---|---|
| Ubiquitous | The system SHALL … | invariants that always hold |
| Event-driven | WHEN *trigger*, the system SHALL … | a discrete stimulus |
| State-driven | WHILE *state*, the system SHALL … | behaviour during a mode |
| Unwanted | IF *condition*, THEN the system SHALL … | error and failure handling |
| Optional | WHERE *feature*, the system SHALL … | conditionally compiled paths (e.g. `TF_POSTGRES_ENABLED`) |

Every requirement gets an ID. Use SHALL, not "should" or "will".

## The acceptance-criteria rule

**An acceptance criterion a machine cannot check is not an acceptance criterion.**

Every `A<n>` names the command or doctest case that proves it. In this repo that
almost always means a doctest case:

```
A1. A handshake with an unresponsive peer returns after ~30s, not never
    — verified by: test/tls_test.h "handshake times out"  (via scripts/gate.py)
```

If something genuinely can only be checked by eye — a UI layout, a colour — mark
it `— verified by: manual inspection` and say what you looked at. Do not disguise
it as automated.

**A criterion must be checkable without a commit.** Neither `/plan` nor `/run`
can commit — only `/sync` can, by permission. So a criterion phrased against
`origin/master...HEAD`, or any revision range, is unverifiable at the moment it
is supposed to be verified: the commits do not exist yet. This produced a real
false finding once, when a verifier substituted `git diff origin/master` and
conflated earlier branch commits with the session's own work. Phrase criteria
against the **working tree**, a doctest case, or a command's output.

**State what the command prints when it fails.** Four audit rounds on one branch
died on the same defect: a criterion naming a command whose output cannot
distinguish pass from fail — `git diff` with no revision, a format step over an
empty file set, a row "changed" observed via `--name-only`. If you cannot say
what failure looks like, the criterion cannot fail, and a criterion that cannot
fail is decoration.

```
A3. A tool without the matching grant is refused
    — verified by: test/permissions_test.h "[permissions] ungranted tool call is denied"
      (via scripts/gate.py). Fails as: doctest reports the case failed and the
      test-case count in GATE SUMMARY drops.
```

## Tiers

| Tier | Shape | Flow |
|---|---|---|
| **S** | one file, no design decision, covered by existing tests | `/plan` writes the SPEC, **audit skipped**, straight to `/run` |
| **M** | default | full plan → audit → run → verify |
| **L** | crosses subsystems, or carries a design decision | full flow, plus Alternatives considered + Rollback, plus a `D-NNN` entry in `docs/notes/decisions.md` |

Tier S is claimed on evidence, not convenience. A change that touches more than
one file, or that anyone would want to discuss, is not Tier S. When in doubt it
is M.

## progress.md

Append-only. Newest entry at the bottom. Never rewrite history — a stale entry
that was later contradicted is itself information.

```markdown
## 2026-08-28 — plan
Request (verbatim): "handshake should time out after 30s instead of hanging"
SPEC written, tier M, 5 requirements / 3 acceptance criteria.
Auditor round 1: DEBT — A2 named no verifying command; R3 "promptly" ambiguous.
Auditor round 2: PASS (nit: consider a fuzz case for partial handshakes).

## 2026-08-28 — run
Added test/tls_test.h with 3 cases; confirmed failing for the right reason:
  [doctest] assertions: 100 | 94 passed | 6 failed
Implemented src/net/tls_session.cpp timeout path.

### verifier report
<the verifier's five-section report, pasted verbatim>
```

## Relationship to the tracker

`docs/status/tracker.md` remains the single source of truth for **what work
exists**. A SPEC is the detail for **one** in-flight branch. Reference the task
ID in the SPEC's Context; do not duplicate the task board into `.spec/`.
