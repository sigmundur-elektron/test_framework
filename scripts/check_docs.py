#!/usr/bin/env python3
"""Referential integrity for the docs/ coordination workspace. (T-050)

The workspace is held together by conventions no tool enforced: tasks are
`T-NNN` in status/tracker.md, decisions are `D-NNN` in notes/decisions.md, and
everything else cites them. Nothing checked that a cited ID exists, so
docs/plans/mvp-roadmap.md has referenced T-012 and D-002 - a task and a decision
that were never written - since 2025.

It also guards the failure this workspace keeps reproducing: a measured number
copied into prose, which then drifts. scripts/baseline.json is the single source
of truth for those, and gate.py checks it automatically; a hardcoded count in a
document has nothing keeping it honest. Historical records are exempt, but only
where they say so.

Standard library only, so it runs anywhere without a pip install.

    python scripts/check_docs.py
"""

from __future__ import annotations

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DOCS = REPO / "docs"
TRACKER = DOCS / "status" / "tracker.md"
DECISIONS = DOCS / "notes" / "decisions.md"

# Files permitted to contain frozen measurements, because they are dated,
# append-only records of what was true at a point in time. They must still say
# so: the annotation is what stops a reader quoting them.
HISTORICAL = {
    "docs/notes/decisions.md",
    "docs/proposals/spec-flow.html",
}
HISTORICAL_MARKERS = ("historical", "do not quote", "not current")

# Prose patterns that should never carry a literal count.
STALE_NUMBER_HINTS = (
    "may_fail assertions:",
    "tracked files under",
    "of 80 tracked",
    "of 81 tracked",
)

# A document that explains an ID does not exist must be able to name it without
# tripping the check. Same shape as any linter's inline suppression:
#
#   <!-- check-docs: allow-missing T-012 D-002 -->
#
# Deliberately explicit and per-file: it records that someone decided, rather
# than silently widening the rule for everyone.
SUPPRESS_MARKER = "check-docs: allow-missing"


def suppressed_ids(text: str) -> set[str]:
    """IDs a file has explicitly declared as known-missing."""
    out: set[str] = set()
    start = 0
    while True:
        i = text.find(SUPPRESS_MARKER, start)
        if i == -1:
            return out
        end = text.find("\n", i)
        segment = text[i + len(SUPPRESS_MARKER):end if end != -1 else len(text)]
        for raw in segment.replace("-->", " ").split():
            token = raw.strip("`*,.;:")
            if len(token) > 2 and token[1] == "-" and token[2:].isdigit():
                out.add(token)
        start = i + 1


class Report:
    def __init__(self) -> None:
        self.failures: list[tuple[str, str]] = []
        self.warnings: list[tuple[str, str]] = []
        self.checked = 0

    def fail(self, where: str, msg: str) -> None:
        self.failures.append((where, msg))

    def warn(self, where: str, msg: str) -> None:
        self.warnings.append((where, msg))


def rel(p: Path) -> str:
    return p.relative_to(REPO).as_posix()


def doc_files() -> list[Path]:
    out = [p for p in DOCS.rglob("*.md")] + [p for p in DOCS.rglob("*.html")]
    return sorted(p for p in out if "node_modules" not in p.parts)


def find_ids(text: str, prefix: str) -> set[str]:
    """Collect PREFIX-NNN tokens by scanning. No regex."""
    found: set[str] = set()
    needle = prefix + "-"
    start = 0
    while True:
        i = text.find(needle, start)
        if i == -1:
            return found
        j = i + len(needle)
        digits = ""
        while j < len(text) and text[j].isdigit() and len(digits) < 4:
            digits += text[j]
            j += 1
        if len(digits) == 3:
            found.add(f"{prefix}-{digits}")
        start = i + 1


def defined_ids(path: Path, prefix: str, table_row: bool) -> tuple[set[str], list[str]]:
    """IDs *declared* in a source-of-truth file, plus any declared twice."""
    ids: list[str] = []
    if not path.exists():
        return set(), []
    for line in path.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if table_row:
            # tracker rows look like: | T-045 | description | ...
            if not s.startswith("|"):
                continue
            cell = s.split("|")[1].strip() if s.count("|") >= 2 else ""
            token = cell.strip("`* ")
        else:
            # decisions headings look like: ## D-009 - title
            if not s.startswith("## "):
                continue
            token = s[3:].split()[0].strip("`*") if len(s) > 3 else ""
        if token.startswith(prefix + "-") and token[len(prefix) + 1:].isdigit():
            ids.append(token)
    dupes = sorted({i for i in ids if ids.count(i) > 1})
    return set(ids), dupes


def check_ids(rep: Report) -> None:
    print("=== task / decision references ===")
    tasks, task_dupes = defined_ids(TRACKER, "T", table_row=True)
    decisions, dec_dupes = defined_ids(DECISIONS, "D", table_row=False)
    print(f"  {len(tasks)} task IDs in {rel(TRACKER)}")
    print(f"  {len(decisions)} decision IDs in {rel(DECISIONS)}")

    for d in task_dupes:
        rep.fail(rel(TRACKER), f"{d} appears in more than one row")
    for d in dec_dupes:
        rep.fail(rel(DECISIONS), f"{d} has more than one heading")

    for path in doc_files():
        text = path.read_text(encoding="utf-8")
        where = rel(path)
        allowed = suppressed_ids(text)
        for tid in sorted(find_ids(text, "T")):
            if tid not in tasks and tid not in allowed:
                rep.fail(where, f"references {tid}, which no row in {rel(TRACKER)} defines")
        for did in sorted(find_ids(text, "D")):
            if did not in decisions and did not in allowed:
                rep.fail(where, f"references {did}, which no heading in {rel(DECISIONS)} defines")
        for sid in sorted(allowed & (tasks | decisions)):
            rep.warn(where, f"suppresses {sid} as missing, but it now exists; drop the suppression")


def check_frontmatter(rep: Report) -> None:
    print()
    print("=== frontmatter ===")
    for path in sorted(DOCS.rglob("*.md")):
        if "node_modules" in path.parts:
            continue
        rep.checked += 1
        lines = path.read_text(encoding="utf-8").splitlines()
        where = rel(path)
        if not lines or lines[0].strip() != "---":
            rep.warn(where, "no YAML frontmatter block (docs/README.md asks for one)")
            continue
        block: list[str] = []
        for line in lines[1:]:
            if line.strip() == "---":
                break
            block.append(line)
        else:
            rep.fail(where, "frontmatter opened with --- but never closed")
            continue
        keys = {ln.split(":", 1)[0].strip() for ln in block if ":" in ln}
        if "last-updated" not in keys:
            rep.warn(where, "frontmatter has no 'last-updated'")
    print(f"  checked {rep.checked} markdown file(s)")


def check_links(rep: Report) -> None:
    print()
    print("=== relative links ===")
    broken = 0
    for path in sorted(DOCS.rglob("*.md")):
        if "node_modules" in path.parts:
            continue
        text = path.read_text(encoding="utf-8")
        where = rel(path)
        start = 0
        while True:
            open_paren = text.find("](", start)
            if open_paren == -1:
                break
            close = text.find(")", open_paren)
            start = open_paren + 2
            if close == -1:
                continue
            target = text[open_paren + 2:close].strip()
            if not target or target.startswith(("http://", "https://", "#", "mailto:")):
                continue
            target = target.split("#", 1)[0]
            if not target:
                continue
            if not (path.parent / target).exists():
                rep.fail(where, f"broken link: {target}")
                broken += 1
    if not broken:
        print("  ok  all relative links resolve")


def check_stale_numbers(rep: Report) -> None:
    print()
    print("=== hardcoded measurements ===")
    hits = 0
    for path in doc_files():
        where = rel(path)
        text = path.read_text(encoding="utf-8")
        lowered = text.lower()
        exempt = where in HISTORICAL
        if exempt and not any(m in lowered for m in HISTORICAL_MARKERS):
            rep.fail(
                where,
                "is exempt from the hardcoded-measurement check but carries no "
                "'historical' / 'do not quote' annotation; the exemption is only "
                "sound while it says so",
            )
        for n, line in enumerate(text.splitlines(), 1):
            low = line.lower()
            for hint in STALE_NUMBER_HINTS:
                if hint not in low:
                    continue
                if not any(ch.isdigit() for ch in line):
                    continue
                hits += 1
                if not exempt:
                    rep.fail(
                        where,
                        f"line {n}: hardcoded measurement near '{hint}'. These live "
                        f"in scripts/baseline.json and gate.py checks them; a copy "
                        f"in prose drifts.",
                    )
    if hits:
        print(f"  {hits} measurement mention(s); non-exempt ones are failures above")
    else:
        print("  ok  no hardcoded measurements outside historical records")


def main() -> int:
    if not DOCS.is_dir():
        sys.exit(f"missing {rel(DOCS)}")
    rep = Report()
    check_ids(rep)
    check_frontmatter(rep)
    check_links(rep)
    check_stale_numbers(rep)

    print()
    print("=== SUMMARY ===")
    for where, msg in rep.warnings:
        print(f"  WARN  {where} : {msg}")
    for where, msg in rep.failures:
        print(f"  FAIL  {where} : {msg}")
    print()
    if rep.failures:
        print(f"DOCS: {len(rep.failures)} PROBLEM(S)")
        return 1
    suffix = f" ({len(rep.warnings)} warning(s))" if rep.warnings else ""
    print(f"DOCS: OK{suffix}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
