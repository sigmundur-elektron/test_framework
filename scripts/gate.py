#!/usr/bin/env python3
"""spec-flow quality gate for test_framework.

Runs the project's quality checks in a fixed order and prints a machine-readable
summary with the exact command and exit code for every step. This script IS the
evidence: agents must not re-implement the steps by hand.

    1. configure  cmake --preset <preset>       (skipped when the cache exists)
    2. build      cmake --build out/build/<preset>
    3. test       out/build/<preset>/test[.exe] --test   (doctest, built into the app)
    4. format     clang-format --dry-run -Werror         (changed files by default)

Fail-fast policy
----------------
A failed *configure* stops the run outright: there is no build system to build,
no binary to test, and anything printed afterwards would describe a stale tree
rather than the change under test. A failed *build* skips the tests (there is no
binary) but still runs the format check, which is independent and cheap, so one
invocation still tells you everything that is wrong.

Baseline
--------
scripts/baseline.json holds the measured facts of a known-good run. This script
parses the doctest summary into real numbers and compares them, printing a
BASELINE: line in the summary. That file is the single source of truth: prose
copies of these numbers in skills and docs drifted three ways before it existed
(a verifier was told may_fail was 4 when it was 3, and reported a false finding
on every run). Update it deliberately with --update-baseline, never silently.

A *drop* in coverage or a change in may_fail is a gate failure. Growth is not:
it prints AHEAD and tells you to re-record.

Portability
-----------
Toolchain discovery is per-platform and isolated in find_toolchain(). On Windows
that means vswhere plus the MSVC developer environment, because cl.exe will not
run without INCLUDE/LIB. On Linux and macOS it means cmake, ninja and
clang-format from PATH, which makes the linux-debug and macos-debug presets in
CMakePresets.json reachable. Only the Windows path is exercised today; the others
are declared and untested. See T-033.

Standard library only. No dependencies.

    python scripts/gate.py
    python scripts/gate.py --scope branch
    python scripts/gate.py --skip-format
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
BASELINE_PATH = REPO / "scripts" / "baseline.json"

SKIPPED = -1  # distinguishes "not run" from "ran and failed" in the summary

DEFAULT_PRESET = {
    "Windows": "x64-debug",
    "Linux": "linux-debug",
    "Darwin": "macos-debug",
}

SOURCE_SUFFIXES = (".cpp", ".h", ".hpp", ".cc", ".c")
SOURCE_ROOTS = ("src/", "test/")


@dataclass
class Step:
    name: str
    command: str
    exit_code: int
    detail: str = ""

    @property
    def verdict(self) -> str:
        if self.exit_code == 0:
            return "PASS"
        if self.exit_code == SKIPPED:
            return "SKIP"
        return "FAIL"


@dataclass
class Toolchain:
    cmake: str
    clang_format: str
    env: dict[str, str] = field(default_factory=lambda: dict(os.environ))
    notes: list[str] = field(default_factory=list)


# --------------------------------------------------------------------- output


def section(title: str) -> None:
    print()
    print(f"=== {title} " + "=" * max(4, 62 - len(title)))


def run(cmd: list[str], env: dict[str, str], echo: str | None = None) -> tuple[int, list[str]]:
    """Run a command, stream its output, and return (exit_code, lines)."""
    print(f"$ {echo or ' '.join(cmd)}")
    proc = subprocess.run(
        cmd, cwd=REPO, env=env, text=True, encoding="utf-8", errors="replace",
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    lines = proc.stdout.splitlines() if proc.stdout else []
    for line in lines:
        print(line)
    return proc.returncode, lines


# ---------------------------------------------------------------- toolchain


def msvc_environment(vs_root: Path) -> dict[str, str]:
    """Capture the MSVC developer environment. cl.exe needs INCLUDE/LIB/PATH."""
    vcvars = vs_root / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    if not vcvars.exists():
        raise FileNotFoundError(f"vcvarsall.bat not found at {vcvars}")
    # Passed as a single string with shell=True: a list would go through
    # list2cmdline, which escapes the inner quotes and breaks the `call`.
    proc = subprocess.run(
        f'"{vcvars}" x64 >nul && set',
        shell=True, capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"vcvarsall.bat failed (exit {proc.returncode}): "
            f"{(proc.stderr or proc.stdout or '').strip()}"
        )
    env = dict(os.environ)
    for line in proc.stdout.splitlines():
        key, sep, value = line.partition("=")
        if sep and key:
            env[key] = value
    if "INCLUDE" not in env:
        raise RuntimeError(
            "vcvarsall.bat ran but INCLUDE is unset; the MSVC environment did not load"
        )
    return env


def find_toolchain() -> Toolchain:
    system = platform.system()

    if system == "Windows":
        vs_root = None
        vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) \
            / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
        if vswhere.exists():
            proc = subprocess.run(
                [str(vswhere), "-latest", "-products", "*",
                 "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                 "-property", "installationPath"],
                capture_output=True, text=True,
            )
            found = proc.stdout.strip().splitlines()
            if found:
                vs_root = Path(found[0])
        if vs_root is None or not vs_root.exists():
            sys.exit("Visual Studio with the C++ toolset was not found (vswhere returned nothing).")

        cmake = vs_root / "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
        clang_format = vs_root / "VC/Tools/Llvm/x64/bin/clang-format.exe"
        for tool in (cmake, clang_format):
            if not tool.exists():
                sys.exit(f"missing tool: {tool}")
        return Toolchain(str(cmake), str(clang_format), msvc_environment(vs_root),
                         [f"vs: {vs_root}"])

    # Linux / macOS: everything comes from PATH.
    missing = [t for t in ("cmake", "clang-format") if shutil.which(t) is None]
    if missing:
        sys.exit(f"missing tool(s) on PATH: {', '.join(missing)}")
    return Toolchain(shutil.which("cmake"), shutil.which("clang-format"),
                     dict(os.environ), [f"platform: {system} (untested path, T-033)"])


# -------------------------------------------------------------------- scope


def git(args: list[str]) -> list[str]:
    proc = subprocess.run(["git", *args], cwd=REPO, capture_output=True, text=True)
    if proc.returncode != 0:
        return []
    return [ln for ln in proc.stdout.splitlines() if ln.strip()]


def default_base_ref() -> str:
    """The branch this repo actually forks from.

    Hardcoding origin/main was wrong here: this repo's base is master, so
    --scope branch silently degraded to 'modified vs HEAD' -- the *weakest*
    scope -- at exactly the moment (pre-PR) the widest one was wanted. Ask git
    what origin/HEAD points at instead of guessing. (T-041)
    """
    ref = git(["symbolic-ref", "--short", "refs/remotes/origin/HEAD"])
    if ref:
        return ref[0]
    for candidate in ("origin/master", "origin/main"):
        if git(["rev-parse", "--verify", "--quiet", candidate]):
            return candidate
    return "origin/master"


def files_in_scope(scope: str, base_ref: str) -> tuple[list[str], str, bool]:
    """Returns (files, description, resolved). resolved is False when a branch
    scope could not find its base ref -- the caller must fail rather than
    quietly fall back to a narrower scope."""
    resolved = True
    if scope == "changed":
        paths = git(["diff", "--name-only", "HEAD"]) + \
                git(["ls-files", "--others", "--exclude-standard"])
        desc = "modified vs HEAD + untracked"
    elif scope == "branch":
        merge_base = git(["merge-base", "HEAD", base_ref])
        if merge_base:
            paths = git(["diff", "--name-only", merge_base[0]])
            desc = f"changed vs {base_ref} ({merge_base[0][:9]})"
        else:
            paths = []
            desc = f"base ref '{base_ref}' did not resolve"
            resolved = False
    else:
        paths = git(["ls-files", "src/*", "test/*"])
        desc = "all tracked files under src/ and test/"

    targets = sorted({
        p for p in paths
        if p.startswith(SOURCE_ROOTS) and p.endswith(SOURCE_SUFFIXES)
        and (REPO / p).exists()
    })
    return targets, desc, resolved


# ------------------------------------------------------------------ baseline


def parse_counts(line: str, label: str) -> dict[str, int]:
    """Turn a doctest summary line into real numbers. No regex.

    "[doctest] test cases:  26 |  26 passed | 0 failed | 0 skipped"
      -> {"total": 26, "passed": 26, "failed": 0, "skipped": 0}
    """
    if label not in line:
        return {}
    parts = [p.strip() for p in line.split(label, 1)[1].split("|") if p.strip()]
    out: dict[str, int] = {}
    if parts and parts[0].isdigit():
        out["total"] = int(parts[0])
    for part in parts[1:]:
        bits = part.split()
        if len(bits) == 2 and bits[0].isdigit():
            out[bits[1]] = int(bits[0])
    return out


def parse_doctest(out: list[str]) -> dict[str, int]:
    """Extract the structured test metrics from a doctest run."""
    metrics: dict[str, int] = {}
    for line in out:
        stripped = line.strip()
        if not stripped.startswith("[doctest]"):
            continue
        cases = parse_counts(stripped, "test cases:")
        if cases:
            metrics["test_cases"] = cases.get("total", 0)
            metrics["test_cases_passed"] = cases.get("passed", 0)
            metrics["test_cases_failed"] = cases.get("failed", 0)
        asserts = parse_counts(stripped, "assertions:")
        if asserts:
            metrics["assertions"] = asserts.get("total", 0)
            metrics["assertions_passed"] = asserts.get("passed", 0)
            metrics["assertions_failed"] = asserts.get("failed", 0)
    # Assertions inside may_fail cases print as errors but do not fail the run.
    metrics["may_fail"] = sum(1 for ln in out if "marking it as not failed" in ln)
    return metrics


def load_baseline() -> dict | None:
    if not BASELINE_PATH.exists():
        return None
    try:
        return json.loads(BASELINE_PATH.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        print(f"warning: {BASELINE_PATH.name} is not valid JSON ({exc}); not comparing")
        return None


def compare_baseline(metrics: dict[str, int], scope: str) -> tuple[list[str], bool]:
    """Compare measured metrics against the recorded baseline.

    Returns (report_lines, drifted). A drop in coverage, a change in may_fail or
    a new warning is drift and fails the gate. Growth is reported, not punished.
    """
    base = load_baseline()
    if base is None:
        return ([f"BASELINE: NONE (no {BASELINE_PATH.name}; run --update-baseline)"], False)

    drift: list[str] = []
    ahead: list[str] = []
    bt = base.get("test", {})

    # may_fail is exact in both directions: a drop can mean a gap was genuinely
    # closed, or that someone deleted the marker. Both need a human to look.
    if "may_fail" in metrics and "may_fail" in bt and metrics["may_fail"] != bt["may_fail"]:
        drift.append(f"may_fail {metrics['may_fail']}, expected {bt['may_fail']}")

    for key in ("test_cases", "assertions"):
        if key not in metrics or key not in bt:
            continue
        if metrics[key] < bt[key]:
            drift.append(f"{key} {metrics[key]}, expected {bt[key]} (dropped)")
        elif metrics[key] > bt[key]:
            ahead.append(f"{key} {metrics[key]}, was {bt[key]}")

    if "test_cases_failed" in metrics and metrics["test_cases_failed"] > 0:
        drift.append(f"test_cases_failed {metrics['test_cases_failed']}, expected 0")

    warn_base = base.get("build", {}).get("first_party_warnings")
    if warn_base is not None and metrics.get("first_party_warnings", 0) > warn_base:
        drift.append(
            f"first_party_warnings {metrics['first_party_warnings']}, expected {warn_base}"
        )

    # Format debt is only comparable when the whole tree was checked.
    bf = base.get("format", {})
    if scope == "all" and "non_conformant" in metrics and "non_conformant" in bf:
        if metrics["non_conformant"] > bf["non_conformant"]:
            drift.append(
                f"format debt {metrics['non_conformant']}, expected {bf['non_conformant']} (grew)"
            )
        elif metrics["non_conformant"] < bf["non_conformant"]:
            ahead.append(f"format debt {metrics['non_conformant']}, was {bf['non_conformant']}")

    lines: list[str] = []
    if drift:
        lines.append(f"BASELINE: DRIFT ({'; '.join(drift)})")
        lines.append(f"  recorded {BASELINE_PATH.name}: "
                     f"{base.get('measured', {}).get('commit', '?')} on "
                     f"{base.get('measured', {}).get('date', '?')}")
    elif ahead:
        lines.append(f"BASELINE: AHEAD ({'; '.join(ahead)})")
        lines.append("  improvement, not a failure. Re-record with --update-baseline")
    else:
        lines.append("BASELINE: MATCH")
    return lines, bool(drift)


def write_baseline(metrics: dict[str, int], preset: str, scope: str) -> None:
    existing = load_baseline() or {}
    head = git(["rev-parse", "--short", "HEAD"])
    base = {
        "$comment": (
            "Measured facts of a known-good gate run. The single source of truth "
            "for these numbers: do not restate them in prose elsewhere. "
            "Regenerate with: python scripts/gate.py --scope all --update-baseline"
        ),
        "measured": {
            "commit": head[0] if head else "?",
            "date": datetime.date.today().isoformat(),
            "preset": preset,
            "platform": platform.system(),
        },
        "build": {"first_party_warnings": metrics.get("first_party_warnings", 0)},
        "test": {
            k: metrics[k] for k in (
                "test_cases", "test_cases_passed", "test_cases_failed",
                "assertions", "assertions_passed", "assertions_failed", "may_fail",
            ) if k in metrics
        },
        "format": existing.get("format", {}),
    }
    if scope == "all" and "non_conformant" in metrics:
        base["format"] = {
            "non_conformant": metrics["non_conformant"],
            "total": metrics.get("format_total", 0),
            "scope": "all",
            "note": "pre-existing debt, see T-021",
        }
    BASELINE_PATH.write_text(json.dumps(base, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {BASELINE_PATH.relative_to(REPO).as_posix()}")
    if scope != "all":
        print("note: format debt not re-recorded (needs --scope all)")


# --------------------------------------------------------------------- main


def finish(steps: list[Step], reason: str = "", baseline: list[str] | None = None,
           drifted: bool = False) -> int:
    section("GATE SUMMARY")
    for s in steps:
        print(f"{s.verdict:<6} {s.name:<10} exit {s.exit_code:<4} {s.detail}")
    failed = [s for s in steps if s.exit_code > 0]
    print()
    for line in baseline or []:
        print(line)
    if baseline:
        print()
    if not failed and not reason and not drifted:
        print("GATE: PASS")
        return 0
    causes = [s.name for s in failed]
    if drifted:
        causes.append("baseline")
    print(f"GATE: FAIL ({', '.join(causes) or 'unknown'})")
    if reason:
        print(f"stopped: {reason}")
    return 1


def main(argv: list[str] | None = None) -> int:
    system = platform.system()
    ap = argparse.ArgumentParser(description="spec-flow quality gate")
    ap.add_argument("--preset", default=DEFAULT_PRESET.get(system, "x64-debug"))
    ap.add_argument("--scope", choices=("changed", "branch", "all"), default="changed",
                    help="which files the format check covers")
    ap.add_argument("--base-ref", default=None,
                    help="base for --scope branch; defaults to origin/HEAD")
    ap.add_argument("--skip-format", action="store_true")
    ap.add_argument("--reconfigure", action="store_true")
    ap.add_argument("--update-baseline", action="store_true",
                    help="re-record scripts/baseline.json from this run")
    ap.add_argument("--clean", action="store_true",
                    help="wipe the build dir first; slow, rebuilds vendored deps")
    args = ap.parse_args(argv)
    if args.base_ref is None:
        args.base_ref = default_base_ref()

    build_dir = REPO / "out" / "build" / args.preset
    build_arg = f"out/build/{args.preset}"
    steps: list[Step] = []
    metrics: dict[str, int] = {}

    section("toolchain")
    tc = find_toolchain()
    for note in tc.notes:
        print(note)
    print(f"cmake        : {subprocess.run([tc.cmake, '--version'], capture_output=True, text=True).stdout.splitlines()[0]}")
    print(f"clang-format : {subprocess.run([tc.clang_format, '--version'], capture_output=True, text=True).stdout.strip()}")
    print(f"preset       : {args.preset}")
    branch = git(["rev-parse", "--abbrev-ref", "HEAD"])
    head = git(["rev-parse", "--short", "HEAD"])
    print(f"commit       : {head[0] if head else '?'} on {branch[0] if branch else '?'}")
    print(f"base ref     : {args.base_ref}")

    # ---------------------------------------------------------- 1. configure
    if args.clean and build_dir.exists():
        section("clean")
        print(f"removing {build_dir}")
        shutil.rmtree(build_dir)
        args.reconfigure = True

    section("1. configure")
    if args.reconfigure or not (build_dir / "CMakeCache.txt").exists():
        cmd = f"cmake --preset {args.preset}"
        code, _ = run([tc.cmake, "--preset", args.preset], tc.env, cmd)
        steps.append(Step("configure", cmd, code))
        if code != 0:
            for name in ("build", "test", "format"):
                steps.append(Step(name, "(not run)", SKIPPED, "skipped: configure failed"))
            return finish(steps, "cmake configure failed")
    else:
        print("cache present, skipping (--reconfigure to force)")
        steps.append(Step("configure", "(cached)", 0, "skipped: CMakeCache.txt present"))

    # -------------------------------------------------------------- 2. build
    section("2. build")
    cmd = f"cmake --build {build_arg}"
    build_code, build_out = run([tc.cmake, "--build", build_arg], tc.env, cmd)
    warnings = [
        ln for ln in build_out
        if ": warning " in ln and "/external/" not in ln.replace("\\", "/")
    ]
    metrics["first_party_warnings"] = len(warnings)
    steps.append(Step("build", cmd, build_code, f"{len(warnings)} first-party warning(s)"))
    if warnings:
        print()
        print(f"first-party warnings ({len(warnings)}):")
        for w in warnings[:20]:
            print(f"  {w}")

    # --------------------------------------------------------------- 3. test
    section("3. test")
    exe = build_dir / ("test.exe" if system == "Windows" else "test")
    if build_code != 0:
        print("build failed, not running tests")
        steps.append(Step("test", "(not run)", SKIPPED, "skipped: build failed"))
    elif not exe.exists():
        print(f"missing {exe}")
        steps.append(Step("test", "(not run)", 1, "test binary not found after a successful build"))
    else:
        cmd = f"{build_arg}/{exe.name} --test"
        code, out = run([str(exe), "--test"], tc.env, cmd)
        # doctest prints: "[doctest] test cases: 22 | 22 passed | 0 failed | 0 skipped"
        summary = " ;; ".join(
            ln.strip() for ln in out
            if ln.strip().startswith("[doctest]")
            and ("test cases:" in ln or "assertions:" in ln)
        )
        metrics.update(parse_doctest(out))
        steps.append(Step("test", cmd, code,
                          f"{summary} ;; may_fail assertions: {metrics.get('may_fail', 0)}"))

    # ------------------------------------------------------------- 4. format
    section("4. format")
    if args.skip_format:
        print("skipped (--skip-format)")
        steps.append(Step("format", "(not run)", SKIPPED, "skipped: --skip-format"))
    else:
        targets, desc, resolved = files_in_scope(args.scope, args.base_ref)
        print(f"scope: {desc}")
        cmd = f"clang-format --dry-run -Werror ({desc})"
        if not resolved:
            # Silently checking fewer files than asked is how a pre-PR run
            # reports green over an unexamined branch. Fail instead. (T-041)
            print(f"cannot resolve base ref '{args.base_ref}' for --scope branch.")
            print("fetch it, or pass an explicit --base-ref.")
            steps.append(Step("format", cmd, 1, f"unresolved base ref '{args.base_ref}'"))
        else:
            print(f"files: {len(targets)}")
            bad = []
            for rel in targets:
                proc = subprocess.run(
                    [tc.clang_format, "--dry-run", "-Werror", "--", str(REPO / rel)],
                    cwd=REPO, capture_output=True, text=True,
                )
                if proc.returncode != 0:
                    bad.append(rel)
            metrics["non_conformant"] = len(bad)
            metrics["format_total"] = len(targets)
            if not targets:
                print("nothing in scope")
                steps.append(Step("format", cmd, 0, "0 files in scope"))
            else:
                if bad:
                    print(f"non-conformant ({len(bad)}):")
                    for rel in bad:
                        print(f"  {rel}")
                    print()
                    print("fix with:")
                    print(f"  \"{tc.clang_format}\" -i <file>")
                else:
                    print("all in-scope files conform")
                steps.append(Step("format", cmd, 1 if bad else 0,
                                  f"{len(bad)}/{len(targets)} non-conformant"))

    if args.update_baseline:
        section("baseline")
        write_baseline(metrics, args.preset, args.scope)
        return finish(steps)

    baseline_lines, drifted = compare_baseline(metrics, args.scope)
    return finish(steps, baseline=baseline_lines, drifted=drifted)


if __name__ == "__main__":
    sys.exit(main())
