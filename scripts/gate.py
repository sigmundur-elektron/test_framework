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
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

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


def files_in_scope(scope: str, base_ref: str) -> tuple[list[str], str]:
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
            print(f"warning: {base_ref} not found; falling back to HEAD")
            paths = git(["diff", "--name-only", "HEAD"])
            desc = "modified vs HEAD (base ref missing)"
    else:
        paths = git(["ls-files", "src/*", "test/*"])
        desc = "all tracked files under src/ and test/"

    targets = sorted({
        p for p in paths
        if p.startswith(SOURCE_ROOTS) and p.endswith(SOURCE_SUFFIXES)
        and (REPO / p).exists()
    })
    return targets, desc


# --------------------------------------------------------------------- main


def finish(steps: list[Step], reason: str = "") -> int:
    section("GATE SUMMARY")
    for s in steps:
        print(f"{s.verdict:<6} {s.name:<10} exit {s.exit_code:<4} {s.detail}")
    failed = [s for s in steps if s.exit_code > 0]
    print()
    if not failed and not reason:
        print("GATE: PASS")
        return 0
    print(f"GATE: FAIL ({', '.join(s.name for s in failed) or 'unknown'})")
    if reason:
        print(f"stopped: {reason}")
    return 1


def main(argv: list[str] | None = None) -> int:
    system = platform.system()
    ap = argparse.ArgumentParser(description="spec-flow quality gate")
    ap.add_argument("--preset", default=DEFAULT_PRESET.get(system, "x64-debug"))
    ap.add_argument("--scope", choices=("changed", "branch", "all"), default="changed",
                    help="which files the format check covers")
    ap.add_argument("--base-ref", default="origin/main")
    ap.add_argument("--skip-format", action="store_true")
    ap.add_argument("--reconfigure", action="store_true")
    ap.add_argument("--clean", action="store_true",
                    help="wipe the build dir first; slow, rebuilds vendored deps")
    args = ap.parse_args(argv)

    build_dir = REPO / "out" / "build" / args.preset
    build_arg = f"out/build/{args.preset}"
    steps: list[Step] = []

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
        # Assertions inside may_fail cases print as errors but do not fail the run.
        may_fail = sum(1 for ln in out if "marking it as not failed" in ln)
        steps.append(Step("test", cmd, code, f"{summary} ;; may_fail assertions: {may_fail}"))

    # ------------------------------------------------------------- 4. format
    section("4. format")
    if args.skip_format:
        print("skipped (--skip-format)")
        steps.append(Step("format", "(not run)", SKIPPED, "skipped: --skip-format"))
    else:
        targets, desc = files_in_scope(args.scope, args.base_ref)
        print(f"scope: {desc}")
        print(f"files: {len(targets)}")
        bad = []
        for rel in targets:
            proc = subprocess.run(
                [tc.clang_format, "--dry-run", "-Werror", "--", str(REPO / rel)],
                cwd=REPO, capture_output=True, text=True,
            )
            if proc.returncode != 0:
                bad.append(rel)
        cmd = f"clang-format --dry-run -Werror ({desc})"
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

    return finish(steps)


if __name__ == "__main__":
    sys.exit(main())
