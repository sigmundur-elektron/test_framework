#!/usr/bin/env python3
"""Fetch and vendor the opencode config JSON Schema.

The schema is the source of truth for what opencode accepts. Vendoring it means
check_opencode.py validates offline, deterministically, and any change to
opencode's config surface shows up as a reviewable diff instead of silently
altering what the validator enforces.

Remote `$ref`s (e.g. models.dev) are replaced with `true` so validation never
depends on a second network fetch. Every replacement is reported and recorded in
the vendored file's `x-vendored` block, so the loss of fidelity is visible rather
than assumed.

    python scripts/refresh_schema.py
"""

from __future__ import annotations

import datetime as _dt
import hashlib
import json
import sys
import urllib.request
from pathlib import Path

SCHEMA_URL = "https://opencode.ai/config.json"
OUT = Path(__file__).resolve().parent / "schema" / "opencode-config.schema.json"


def neutralize_remote_refs(node: object, found: list[str], path: str = "") -> object:
    """Replace absolute-URL $refs with `true` (accept anything) and record them."""
    if isinstance(node, dict):
        ref = node.get("$ref")
        if isinstance(ref, str) and ref.startswith(("http://", "https://")):
            found.append(f"{path or '<root>'} -> {ref}")
            rest = {k: v for k, v in node.items() if k != "$ref"}
            return neutralize_remote_refs(rest, found, path) if rest else True
        return {
            k: neutralize_remote_refs(v, found, f"{path}/{k}") for k, v in node.items()
        }
    if isinstance(node, list):
        return [neutralize_remote_refs(v, found, f"{path}[{i}]") for i, v in enumerate(node)]
    return node


def main() -> int:
    print(f"fetching {SCHEMA_URL}")
    # The CDN rejects urllib's default User-Agent with HTTP 403.
    req = urllib.request.Request(
        SCHEMA_URL, headers={"User-Agent": "test_framework-spec-flow/1.0"}
    )
    with urllib.request.urlopen(req, timeout=60) as resp:
        raw = resp.read()

    schema = json.loads(raw)
    upstream_sha = hashlib.sha256(raw).hexdigest()

    removed: list[str] = []
    schema = neutralize_remote_refs(schema, removed)

    schema["x-vendored"] = {
        "source": SCHEMA_URL,
        "fetched": _dt.datetime.now(_dt.UTC).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "upstream_sha256": upstream_sha,
        "remote_refs_neutralized": sorted(removed),
        "note": (
            "Absolute-URL $refs were replaced with `true` so validation is offline "
            "and deterministic. Validation is therefore permissive at exactly those "
            "points and nowhere else."
        ),
    }

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(schema, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"wrote {OUT.relative_to(Path(__file__).resolve().parent.parent)}")
    print(f"upstream sha256: {upstream_sha}")
    if removed:
        print(f"neutralized {len(removed)} remote $ref(s):")
        for r in sorted(removed):
            print(f"  {r}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
