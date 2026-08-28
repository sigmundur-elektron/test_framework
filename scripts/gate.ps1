<#
.SYNOPSIS
    Compatibility shim. The gate is now scripts/gate.py.

.DESCRIPTION
    opencode caches .opencode/ at startup, so a session that began before the
    Python port (D-007) still invokes `pwsh -File scripts/gate.ps1`. Deleting
    this file made those sessions fail with "file not found", which reads like a
    broken repository rather than a stale session.

    This shim forwards to the real gate and says why it exists. It takes no
    parameters of its own: everything is passed straight through, so the old
    PowerShell flags will NOT work. That is deliberate — a shim that silently
    translated -SkipFormat into --skip-format would hide the staleness instead of
    surfacing it.

    Delete this file once no long-running session predates D-007. Tracked as T-039.
#>

Write-Host 'scripts/gate.ps1 is a compatibility shim; the gate is now scripts/gate.py.' -ForegroundColor Yellow
Write-Host 'Your opencode session cached a pre-D-007 command template. Restart opencode.' -ForegroundColor Yellow
Write-Host 'Forwarding to: python scripts/gate.py' -ForegroundColor Yellow
Write-Host ''

$python = if (Get-Command python -ErrorAction SilentlyContinue) { 'python' }
elseif (Get-Command python3 -ErrorAction SilentlyContinue) { 'python3' }
else { Write-Error 'python not found on PATH'; exit 90 }

& $python (Join-Path $PSScriptRoot 'gate.py') @args
exit $LASTEXITCODE
