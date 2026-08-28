#!/usr/bin/env python3
"""Structural validation for the spec-flow harness under .opencode/.

opencode loads .opencode/ once at startup and fails *quietly* on several
mistakes. A skill whose `name` does not match its directory is dropped. An
unknown agent frontmatter field is silently routed into `options`. A missing
`description` hides an agent or skill from the model entirely. None of that
raises an error you would notice mid-session.

Design notes
------------
* **No regex, no hand-rolled parsing.** Frontmatter is parsed with PyYAML;
  opencode.json with `json`. If a file is not valid YAML/JSON this reports the
  parser's own error rather than guessing at the content.
* **Schema-driven.** Validation is against the vendored copy of
  https://opencode.ai/config.json (see scripts/refresh_schema.py), so the
  accepted key sets come from opencode itself and cannot rot here.
* **Both directory generations.** opencode reads plural `agents/`, `commands/`,
  `skills/` (current) and singular `agent/`, `command/`, `skill/` (legacy,
  back-compat). Both are checked; the singular form is reported as deprecated.
* **Failure vs warning.** Only things that break or silently disable something
  are failures. Things that are legal but suspicious are warnings. The schema
  declares `PermissionConfig.additionalProperties`, so an undocumented
  permission key is legal (it is a tool-name pattern) and is a warning.

Requires: pyyaml, jsonschema  ->  python -m pip install -r scripts/requirements.txt
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Iterable

try:
    import yaml
except ModuleNotFoundError:
    sys.exit(
        "missing dependency: pyyaml\n"
        "  python -m pip install -r scripts/requirements.txt\n"
        "Refusing to run: an approximate validator that prints OK is worse than none."
    )

try:
    from jsonschema import Draft202012Validator
except ModuleNotFoundError:
    sys.exit(
        "missing dependency: jsonschema\n"
        "  python -m pip install -r scripts/requirements.txt\n"
        "Refusing to run: an approximate validator that prints OK is worse than none."
    )

REPO = Path(__file__).resolve().parent.parent
SCHEMA_PATH = REPO / "scripts" / "schema" / "opencode-config.schema.json"

# opencode reads both generations of directory names. Plural is current; singular
# is kept for backwards compatibility and is reported as deprecated.
DIR_GENERATIONS = {
    "agents": ("agents", "agent"),
    "commands": ("commands", "command"),
    "skills": ("skills", "skill"),
}

# Markdown frontmatter is not exactly AgentConfig. Two documented deltas:
FRONTMATTER_EXTRA = {"name"}  # allowed in frontmatter, absent from AgentConfig
FRONTMATTER_BANNED = {"prompt"}  # the body IS the prompt; setting both is a bug

BUILTIN_AGENTS = {"build", "plan", "general", "explore", "scout",
                  "title", "summary", "compaction"}

SKILL_KEYS = {"name", "description", "license", "compatibility", "metadata"}


class Report:
    def __init__(self) -> None:
        self.failures: list[tuple[str, str]] = []
        self.warnings: list[tuple[str, str]] = []
        self.checked = 0

    def fail(self, where: str, msg: str) -> None:
        self.failures.append((where, msg))

    def warn(self, where: str, msg: str) -> None:
        self.warnings.append((where, msg))

    def mark(self) -> int:
        """Snapshot the failure count so a caller can tell if a file was clean."""
        return len(self.failures)

    def clean_since(self, mark: int) -> bool:
        return len(self.failures) == mark


def load_schema() -> dict[str, Any]:
    if not SCHEMA_PATH.exists():
        sys.exit(
            f"missing vendored schema: {SCHEMA_PATH.relative_to(REPO)}\n"
            "  python scripts/refresh_schema.py"
        )
    return json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))


def subschema(schema: dict[str, Any], pointer: Iterable[str]) -> dict[str, Any]:
    """Resolve a local JSON-pointer path within the vendored schema."""
    node: Any = schema
    for part in pointer:
        node = node[part]
    return node


def validator_for(schema: dict[str, Any], node: dict[str, Any]) -> Draft202012Validator:
    """A validator for `node` that can still resolve #/$defs/... references."""
    merged = dict(node)
    merged["$defs"] = schema["$defs"]
    return Draft202012Validator(merged)


def split_frontmatter(path: Path) -> tuple[dict[str, Any] | None, str, str | None]:
    """Return (frontmatter, body, error). Uses no regex."""
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return None, text, "no --- frontmatter block at the top of the file"
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            block = "\n".join(lines[1:i])
            body = "\n".join(lines[i + 1:])
            try:
                data = yaml.safe_load(block)
            except yaml.YAMLError as exc:
                return None, body, f"invalid YAML frontmatter: {exc}"
            if data is None:
                data = {}
            if not isinstance(data, dict):
                return None, body, "frontmatter is not a mapping"
            return data, body, None
    return None, text, "frontmatter opened with --- but never closed"


def resolve_dirs(root: Path, kind: str, rep: Report) -> list[tuple[Path, bool]]:
    """Return [(dir, is_deprecated_singular)] for every generation present."""
    out = []
    plural, singular = DIR_GENERATIONS[kind]
    for name, deprecated in ((plural, False), (singular, True)):
        d = root / name
        if d.is_dir():
            out.append((d, deprecated))
            if deprecated:
                rel = d.relative_to(REPO).as_posix()
                rep.warn(rel, f"legacy singular directory; opencode prefers '{plural}/'")
    return out


def check_config(schema: dict[str, Any], rep: Report) -> None:
    print("=== opencode.json ===")
    p = REPO / "opencode.json"
    if not p.exists():
        rep.fail("opencode.json", "missing")
        return
    rep.checked += 1
    try:
        cfg = json.loads(p.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        rep.fail("opencode.json", f"invalid JSON: {exc}")
        return

    v = validator_for(schema, subschema(schema, ("$defs", "Config")))
    errors = sorted(v.iter_errors(cfg), key=lambda e: list(e.absolute_path))
    for e in errors:
        loc = "/".join(str(x) for x in e.absolute_path) or "<root>"
        rep.fail("opencode.json", f"{loc}: {e.message}")
    if not errors:
        print("  ok  validates against the vendored opencode config schema")

    if "$schema" not in cfg:
        rep.warn("opencode.json", "no $schema declared; editors cannot autocomplete or lint it")

    # Deprecated keys the schema still accepts.
    for dead, live in (("reference", "references"), ("autoshare", "share"),
                       ("maxSteps", "steps")):
        if dead in cfg:
            rep.warn("opencode.json", f"'{dead}' is deprecated; use '{live}'")
    for tui_key in ("theme", "keybinds", "tui"):
        if tui_key in cfg:
            rep.warn("opencode.json", f"'{tui_key}' belongs in tui.json now; it is auto-migrated")

    check_permissions("opencode.json", cfg.get("permission"), schema, rep)

    for inst in cfg.get("instructions", []) or []:
        if not any(c in inst for c in "*?[") and not (REPO / inst).exists():
            rep.fail("opencode.json", f"instructions file not found: {inst}")

    default_agent = cfg.get("default_agent")
    if default_agent and default_agent not in BUILTIN_AGENTS:
        if not any((d / f"{default_agent}.md").exists()
                   for d, _ in resolve_dirs(REPO / ".opencode", "agents", Report())):
            rep.fail("opencode.json", f"default_agent '{default_agent}' does not exist")


def check_permissions(where: str, perms: Any, schema: dict[str, Any], rep: Report) -> None:
    if not isinstance(perms, dict):
        return
    documented = set(
        subschema(schema, ("$defs", "PermissionConfig"))["anyOf"][1]["properties"]
    )
    # These accept a bare action only, never a pattern map.
    action_only = {
        k for k, v in subschema(schema, ("$defs", "PermissionConfig"))["anyOf"][1][
            "properties"
        ].items()
        if v.get("$ref", "").endswith("PermissionActionConfig")
    }
    for key, value in perms.items():
        if key not in documented:
            rep.warn(
                where,
                f"permission '{key}' is not a documented key. The schema allows it as a "
                f"tool-name pattern, so this is legal but does nothing unless a tool is "
                f"actually named '{key}'. Check for a typo.",
            )
        if key in action_only and isinstance(value, dict):
            rep.fail(where, f"permission '{key}' takes a bare action, not a pattern map")


def check_skills(schema: dict[str, Any], rep: Report) -> None:
    print()
    print("=== skills ===")
    seen: dict[str, str] = {}
    for base, _dep in resolve_dirs(REPO / ".opencode", "skills", rep):
        for d in sorted(p for p in base.iterdir() if p.is_dir()):
            rel = (d / "SKILL.md").relative_to(REPO).as_posix()
            rep.checked += 1
            mark = rep.mark()
            f = d / "SKILL.md"
            if not f.exists():
                rep.fail(rel, "no SKILL.md (the filename must be exactly that, uppercase)")
                continue
            fm, _body, err = split_frontmatter(f)
            if err:
                rep.fail(rel, err)
                continue

            name = fm.get("name")
            if name is None:
                rep.fail(rel, "missing required key: name")
            elif name != d.name:
                rep.fail(rel, f"name '{name}' != directory '{d.name}'; the skill is dropped")
            elif name in seen and seen[name] != rel:
                rep.fail(rel, f"duplicate skill name '{name}', also in {seen[name]}")
            else:
                seen[name] = rel

            if not valid_slug(d.name):
                rep.fail(rel, f"directory '{d.name}' is not lowercase-hyphen-separated")

            desc = fm.get("description")
            if not desc:
                rep.fail(rel, "missing description; the skill is never surfaced to the model")
            elif not isinstance(desc, str):
                rep.fail(rel, "description must be a string")
            elif not 1 <= len(desc) <= 1024:
                rep.fail(rel, f"description is {len(desc)} chars; must be 1-1024")

            meta = fm.get("metadata")
            if meta is not None and (
                not isinstance(meta, dict)
                or not all(isinstance(k, str) and isinstance(v, str) for k, v in meta.items())
            ):
                rep.fail(rel, "metadata must be a string-to-string map")

            for k in set(fm) - SKILL_KEYS:
                rep.warn(rel, f"frontmatter key '{k}' is not recognised and is ignored")
            if rep.clean_since(mark):
                print(f"  ok  {rel}")


def valid_slug(s: str) -> bool:
    """lowercase alphanumeric with single hyphen separators. No regex."""
    if not s or not 1 <= len(s) <= 64:
        return False
    parts = s.split("-")
    return all(p and p.isascii() and p.isalnum() and p.lower() == p for p in parts)


def check_agents(schema: dict[str, Any], rep: Report) -> list[str]:
    print()
    print("=== agents ===")
    agent_schema = subschema(schema, ("$defs", "AgentConfig"))
    allowed = (set(agent_schema["properties"]) | FRONTMATTER_EXTRA) - FRONTMATTER_BANNED
    names: list[str] = []

    for base, _dep in resolve_dirs(REPO / ".opencode", "agents", rep):
        for f in sorted(base.glob("*.md")):
            rel = f.relative_to(REPO).as_posix()
            rep.checked += 1
            mark = rep.mark()
            names.append(f.stem)
            fm, body, err = split_frontmatter(f)
            if err:
                rep.fail(rel, err)
                continue

            for banned in FRONTMATTER_BANNED & set(fm):
                rep.fail(rel, f"do not set '{banned}:' in frontmatter; the body is the prompt")

            unknown = set(fm) - allowed
            for k in unknown:
                rep.warn(rel, f"frontmatter key '{k}' is not recognised; "
                              f"opencode silently routes it into 'options'")

            # Validate the known subset against the real AgentConfig schema.
            known = {k: v for k, v in fm.items() if k in agent_schema["properties"]}
            v = validator_for(schema, agent_schema)
            for e in sorted(v.iter_errors(known), key=lambda e: list(e.absolute_path)):
                loc = "/".join(str(x) for x in e.absolute_path) or "<root>"
                rep.fail(rel, f"{loc}: {e.message}")

            if not fm.get("description"):
                rep.fail(rel, "missing description; other agents cannot discover it")
            if not body.strip():
                rep.fail(rel, "empty body; the body is the agent's prompt")
            model = fm.get("model")
            if isinstance(model, str) and "/" not in model:
                rep.fail(rel, f"model '{model}' has no provider prefix (want 'provider/model-id')")
            if fm.get("hidden") and fm.get("mode") != "subagent":
                rep.warn(rel, "'hidden' only applies to mode: subagent")

            check_permissions(rel, fm.get("permission"), schema, rep)
            if rep.clean_since(mark):
                print(f"  ok  {rel}")
    return names


def check_commands(schema: dict[str, Any], rep: Report, agent_names: list[str]) -> None:
    print()
    print("=== commands ===")
    cmd_schema = dict(
        subschema(schema, ("$defs", "Config", "properties", "command"))["additionalProperties"]
    )
    # In markdown form the body supplies `template`, so it is not required here.
    cmd_schema.pop("required", None)
    known_agents = BUILTIN_AGENTS | set(agent_names)

    for base, _dep in resolve_dirs(REPO / ".opencode", "commands", rep):
        for f in sorted(base.glob("*.md")):
            rel = f.relative_to(REPO).as_posix()
            rep.checked += 1
            mark = rep.mark()
            fm, body, err = split_frontmatter(f)
            if err:
                rep.fail(rel, err)
                continue

            if "template" in fm:
                rep.fail(rel, "do not set 'template:' in frontmatter; the body is the template")
            if not body.strip():
                rep.fail(rel, "empty body; the body IS the prompt and is required")
            if not fm.get("description"):
                rep.warn(rel, "no description; the command shows blank in the TUI")

            v = validator_for(schema, cmd_schema)
            for e in sorted(v.iter_errors(fm), key=lambda e: list(e.absolute_path)):
                loc = "/".join(str(x) for x in e.absolute_path) or "<root>"
                rep.fail(rel, f"{loc}: {e.message}")

            agent = fm.get("agent")
            if agent and agent not in known_agents:
                rep.fail(rel, f"agent: '{agent}' does not exist")

            for token in mentioned_agents(body):
                if token not in known_agents:
                    rep.fail(rel, f"references @{token} but no such agent exists")
            if rep.clean_since(mark):
                print(f"  ok  {rel}")


def mentioned_agents(body: str) -> set[str]:
    """Collect @agent mentions by scanning, not by regex."""
    found: set[str] = set()
    for raw in body.replace("(", " ").replace(")", " ").replace(",", " ").split():
        token = raw.strip("`*_.:;!?'\"")
        if len(token) > 1 and token.startswith("@"):
            candidate = token[1:]
            if valid_slug(candidate):
                found.add(candidate)
    return found


def check_references(rep: Report) -> None:
    print()
    print("=== cross-references ===")
    for f in sorted((REPO / ".opencode").rglob("*.md")):
        if "node_modules" in f.parts:
            continue
        rel = f.relative_to(REPO).as_posix()
        text = f.read_text(encoding="utf-8")
        for raw in text.split():
            token = raw.strip("`*_.:;!?'\"(),")
            if token.startswith("scripts/") and token.endswith((".ps1", ".py")):
                if not (REPO / token).exists():
                    rep.fail(rel, f"references {token} which does not exist")
    print("  ok  script references resolve")


def main() -> int:
    schema = load_schema()
    vend = schema.get("x-vendored", {})
    print(f"schema: vendored {vend.get('fetched', '?')} from {vend.get('source', '?')}")

    rep = Report()
    check_config(schema, rep)
    check_skills(schema, rep)
    agent_names = check_agents(schema, rep)
    check_commands(schema, rep, agent_names)
    check_references(rep)

    print()
    print("=== SUMMARY ===")
    print(f"checked {rep.checked} file(s)")
    for where, msg in rep.warnings:
        print(f"  WARN  {where} : {msg}")
    for where, msg in rep.failures:
        print(f"  FAIL  {where} : {msg}")
    print()
    if rep.failures:
        print(f"HARNESS: {len(rep.failures)} PROBLEM(S)")
        return 1
    suffix = f" ({len(rep.warnings)} warning(s))" if rep.warnings else ""
    print(f"HARNESS: OK{suffix}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
