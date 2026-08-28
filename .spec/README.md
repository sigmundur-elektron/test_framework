# `.spec/` — one SPEC per feature branch

Each feature branch gets its own directory here, named after the branch with
`/` replaced by `-`:

```
.spec/
  README.md                 this file (lives on every branch)
  feat-tls-timeout/
    spec.md                 the specification — source of truth for the branch
    progress.md             append-only log of what ran and what it printed
```

Resolve the directory for the current branch with:

```powershell
(git rev-parse --abbrev-ref HEAD) -replace '/','-'
```

## Why per-branch directories

Two branches never collide, and merging a branch into `main` never produces a
conflict in the SPEC. Merge selectively: keep the SPEC if it documents something
worth keeping, drop it in the squash if it does not.

## Conventions

- **Both files are committed.** The audit trail is the point — see D-005.
- `spec.md` follows the template in `.opencode/skills/spec-format/SKILL.md`.
  `status` moves `draft` → `audited` → `in-progress` → `done`.
- `progress.md` is **append-only**. Never rewrite an earlier entry; a claim that
  was later contradicted is itself information.
- Verifier reports are pasted into `progress.md` **verbatim**, never summarised.
- This directory is outside `src/`, so `file(GLOB_RECURSE src/*.cpp)` in
  `CMakeLists.txt` never sweeps it into the build.

## Relationship to the tracker

[`docs/status/tracker.md`](../docs/status/tracker.md) stays the single source of
truth for *what work exists*. A SPEC is the detail for *one in-flight branch*.
Reference the `T-NNN` in the SPEC's Context; do not duplicate the task board here.

## Commands

| | |
|---|---|
| `/plan "<description>"` | writes `spec.md`, then dispatches `@spec-auditor` to grade it |
| `/run` | implements it, then dispatches `@verifier` to produce the evidence |
| `/gate` | runs `scripts/gate.py` and reports, at any time |
