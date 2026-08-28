<#
.SYNOPSIS
    spec-flow quality gate for test_framework.

.DESCRIPTION
    Runs the project's quality checks in a fixed order and prints a machine-readable
    summary with the exact command and exit code for every step. This script IS the
    evidence: agents must not re-implement the steps by hand.

    Steps:
      1. configure  cmake --preset <preset>            (skipped if cache exists, unless -Reconfigure)
      2. build      cmake --build out/build/<preset>
      3. test       out/build/<preset>/test.exe --test  (doctest, embedded in the app)
      4. format     clang-format --dry-run -Werror       (changed files by default)

.PARAMETER Preset
    CMake configure preset. Default: x64-debug.

.PARAMETER Scope
    Which files the format check covers.
      Changed  (default) files modified vs. HEAD, plus untracked files
      Branch   files changed vs. the merge-base with -BaseRef
      All      every file under src/ and test/  (currently 44/80 fail; see T-021)

.PARAMETER BaseRef
    Base ref for -Scope Branch. Default: origin/main.

.PARAMETER SkipFormat
    Skip the format check.

.PARAMETER Reconfigure
    Force cmake configure even when the build cache exists.

.PARAMETER Clean
    Wipe out/build/<preset> before configuring. Implies -Reconfigure. Slow (~10 min).

.EXAMPLE
    pwsh -File scripts/gate.ps1
.EXAMPLE
    pwsh -File scripts/gate.ps1 -Scope Branch -BaseRef origin/main
#>
[CmdletBinding()]
param(
    [string]$Preset = 'x64-debug',
    [ValidateSet('Changed', 'Branch', 'All')]
    [string]$Scope = 'Changed',
    [string]$BaseRef = 'origin/main',
    [switch]$SkipFormat,
    [switch]$Reconfigure,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "out/build/$Preset"

$Steps = [System.Collections.Generic.List[object]]::new()
function Add-Step([string]$Name, [string]$Command, [int]$ExitCode, [string]$Detail = '') {
    $Steps.Add([pscustomobject]@{ Name = $Name; Command = $Command; ExitCode = $ExitCode; Detail = $Detail })
}
function Write-Section([string]$Title) {
    Write-Host ''
    Write-Host "=== $Title " -NoNewline
    Write-Host ('=' * [Math]::Max(4, 62 - $Title.Length))
}

# ---------------------------------------------------------------- toolchain
Write-Section 'toolchain'

$VsRoot = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath 2>$null
if (-not $VsRoot) { $VsRoot = 'C:\Program Files\Microsoft Visual Studio\2022\Professional' }
if (-not (Test-Path -LiteralPath $VsRoot)) {
    Write-Error "Visual Studio not found. Looked at: $VsRoot"
    exit 90
}

$CMake       = Join-Path $VsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ClangFormat = Join-Path $VsRoot 'VC\Tools\Llvm\x64\bin\clang-format.exe'
foreach ($t in @($CMake, $ClangFormat)) {
    if (-not (Test-Path -LiteralPath $t)) { Write-Error "Missing tool: $t"; exit 90 }
}

# cl.exe needs INCLUDE/LIB/PATH from the VS developer environment.
Import-Module (Join-Path $VsRoot 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VsRoot -DevCmdArguments '-arch=x64 -host_arch=x64' -SkipAutomaticLocation | Out-Null
Set-Location -LiteralPath $RepoRoot

Write-Host "vs           : $VsRoot"
Write-Host "cmake        : $(& $CMake --version | Select-Object -First 1)"
Write-Host "clang-format : $(& $ClangFormat --version)"
Write-Host "preset       : $Preset"
Write-Host "commit       : $(git rev-parse --short HEAD) on $(git rev-parse --abbrev-ref HEAD)"

# ---------------------------------------------------------------- 1. configure
if ($Clean -and (Test-Path -LiteralPath $BuildDir)) {
    Write-Section 'clean'
    Write-Host "removing $BuildDir"
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
    $Reconfigure = $true
}

$CacheExists = Test-Path -LiteralPath (Join-Path $BuildDir 'CMakeCache.txt')
if ($Reconfigure -or -not $CacheExists) {
    Write-Section '1. configure'
    $cmd = "cmake --preset $Preset"
    Write-Host "`$ $cmd"
    & $CMake --preset $Preset
    Add-Step 'configure' $cmd $LASTEXITCODE
    if ($LASTEXITCODE -ne 0) { $Skipped = $true }
}
else {
    Write-Section '1. configure'
    Write-Host 'cache present, skipping (-Reconfigure to force)'
    Add-Step 'configure' "(cached)" 0 'skipped: CMakeCache.txt present'
}

# ---------------------------------------------------------------- 2. build
Write-Section '2. build'
$cmd = "cmake --build out/build/$Preset"
Write-Host "`$ $cmd"
$buildOut = & $CMake --build "out/build/$Preset" 2>&1
$buildExit = $LASTEXITCODE
$buildOut | ForEach-Object { Write-Host $_ }

# MSVC warnings on first-party code only; vendored deps under external/ are noise.
$warnings = @($buildOut | Where-Object { $_ -match ': warning [A-Z]+\d+' -and $_ -notmatch '[\\/]external[\\/]' })
Add-Step 'build' $cmd $buildExit "$($warnings.Count) first-party warning(s)"
if ($warnings.Count -gt 0) {
    Write-Host ''
    Write-Host "first-party warnings ($($warnings.Count)):"
    $warnings | Select-Object -First 20 | ForEach-Object { Write-Host "  $_" }
}

# ---------------------------------------------------------------- 3. test
Write-Section '3. test'
$exe = Join-Path $BuildDir 'test.exe'
if ($buildExit -ne 0) {
    Write-Host 'build failed, not running tests'
    Add-Step 'test' '(not run)' 1 'skipped: build failed'
    $testExit = 1
}
elseif (-not (Test-Path -LiteralPath $exe)) {
    Write-Host "missing $exe"
    Add-Step 'test' '(not run)' 1 'skipped: test.exe not found'
    $testExit = 1
}
else {
    $cmd = "out/build/$Preset/test.exe --test"
    Write-Host "`$ $cmd"
    $testOut = & $exe --test 2>&1
    $testExit = $LASTEXITCODE
    $testOut | ForEach-Object { Write-Host $_ }

    # doctest summary: "test cases: 22 | 22 passed | 0 failed | 0 skipped"
    $summary = ($testOut | Select-String -Pattern '\[doctest\] (test cases|assertions):' | ForEach-Object { $_.ToString().Trim() }) -join ' ;; '
    # Assertions inside may_fail test cases still print as failures but do not fail the run.
    $mayFail = @($testOut | Select-String -Pattern 'marking it as not failed').Count
    Add-Step 'test' $cmd $testExit "$summary ;; may_fail assertions: $mayFail"
}

# ---------------------------------------------------------------- 4. format
Write-Section '4. format'
if ($SkipFormat) {
    Write-Host 'skipped (-SkipFormat)'
    Add-Step 'format' '(not run)' 0 'skipped: -SkipFormat'
}
else {
    switch ($Scope) {
        'Changed' {
            $paths = @(git diff --name-only HEAD) + @(git ls-files --others --exclude-standard)
            $scopeDesc = 'modified vs HEAD + untracked'
        }
        'Branch' {
            $base = (git merge-base HEAD $BaseRef 2>$null)
            if (-not $base) { Write-Host "warning: $BaseRef not found, falling back to HEAD"; $base = 'HEAD' }
            $paths = @(git diff --name-only $base)
            $scopeDesc = "changed vs $BaseRef ($base)"
        }
        'All' {
            $paths = @(git ls-files 'src/*' 'test/*')
            $scopeDesc = 'all tracked files under src/ and test/'
        }
    }

    $targets = @($paths |
        Where-Object { $_ -match '^(src|test)/' -and $_ -match '\.(cpp|h|hpp|cc|c)$' } |
        Sort-Object -Unique |
        Where-Object { Test-Path -LiteralPath (Join-Path $RepoRoot $_) })

    Write-Host "scope: $scopeDesc"
    Write-Host "files: $($targets.Count)"

    $bad = [System.Collections.Generic.List[string]]::new()
    foreach ($f in $targets) {
        & $ClangFormat --dry-run -Werror -- (Join-Path $RepoRoot $f) 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { $bad.Add($f) }
    }

    if ($targets.Count -eq 0) {
        Write-Host 'nothing in scope'
        Add-Step 'format' "clang-format --dry-run -Werror ($scopeDesc)" 0 '0 files in scope'
        $fmtExit = 0
    }
    else {
        $fmtExit = if ($bad.Count -gt 0) { 1 } else { 0 }
        if ($bad.Count -gt 0) {
            Write-Host "non-conformant ($($bad.Count)):"
            $bad | ForEach-Object { Write-Host "  $_" }
            Write-Host ''
            Write-Host 'fix with:'
            Write-Host "  & '$ClangFormat' -i <file>"
        }
        else {
            Write-Host 'all in-scope files conform'
        }
        Add-Step 'format' "clang-format --dry-run -Werror ($scopeDesc)" $fmtExit "$($bad.Count)/$($targets.Count) non-conformant"
    }
}

# ---------------------------------------------------------------- summary
Write-Section 'GATE SUMMARY'
$Steps | ForEach-Object {
    $verdict = if ($_.ExitCode -eq 0) { 'PASS' } else { 'FAIL' }
    '{0,-6} {1,-10} exit {2,-4} {3}' -f $verdict, $_.Name, $_.ExitCode, $_.Detail
}

$failed = @($Steps | Where-Object { $_.ExitCode -ne 0 })
Write-Host ''
if ($failed.Count -eq 0) {
    Write-Host 'GATE: PASS'
    exit 0
}
else {
    Write-Host "GATE: FAIL ($($failed.Name -join ', '))"
    exit 1
}
