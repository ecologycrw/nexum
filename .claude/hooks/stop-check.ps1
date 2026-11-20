# Claude Code Stop hook: before the agent finishes a turn with uncommitted changes,
# runs the documentation and red-line checks (and the build, if code changed). Exit code 2 sends the problems
# back to the agent, which must fix them before finishing. This file is a protected "trava".

[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
if ($env:CLAUDE_PROJECT_DIR) { Set-Location -LiteralPath $env:CLAUDE_PROJECT_DIR }

try {
    $payload = [Console]::In.ReadToEnd() | ConvertFrom-Json
}
catch {
    $payload = $null
}

# Avoid an endless loop: when this hook already sent the agent back once, let it stop.
if ($payload -and $payload.stop_hook_active) { exit 0 }

function Invoke-Python {
    param([string[]] $Arguments)
    if (Get-Command py -ErrorAction SilentlyContinue) {
        & py -3 @Arguments 2>&1
    }
    else {
        & python3 @Arguments 2>&1
    }
}

$changed = @(git status --porcelain -- Engine Kit Games Tools tests scripts docs specs templates cmake .claude AGENTS.md CLAUDE.md CMakeLists.txt 2>$null)
if ($changed.Count -eq 0) { exit 0 }

$problems = @()

$checks = Invoke-Python @('scripts/checks/run.py', 'docs', 'terms')
if ($LASTEXITCODE -ne 0) {
    $problems += 'Documentation and red-line checks failed:'
    $problems += @($checks | ForEach-Object { "  $_" })
}

$codeChanged = @($changed | Where-Object { $_ -match '(\.(cpp|hpp|cmake)|CMakeLists\.txt)"?\s*$' }).Count -gt 0
if ($codeChanged) {
    $build = Invoke-Python @('scripts/dev.py', 'build')
    if ($LASTEXITCODE -ne 0) {
        $problems += 'Build failed:'
        $problems += @($build | ForEach-Object { "$_" } | Where-Object { $_ -match '\berror\b' -and $_ -notmatch 'system_error' } | Select-Object -Unique -First 20 | ForEach-Object { "  $_" })
    }
}

if ($problems.Count -gt 0) {
    [Console]::Error.WriteLine("stop-check.ps1: fix these problems before finishing (or explain to the owner why you cannot):`n" + ($problems -join "`n"))
    exit 2
}

exit 0
