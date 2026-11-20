# Tests for .claude/hooks/guard-bash.ps1. Runs in the CI (Windows) and locally through py scripts/dev.py pytests.
# This file is a protected "trava".

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\..\.claude\hooks\guard-bash.ps1')

$blocked = @(
    'git push --force origin fatia/0000-x',
    'git push -f',
    'git push -uf origin fatia/0000-x',
    'git push --force-with-lease',
    'git push origin +fatia/0000-x',
    'git push origin main',
    'git push -u origin main',
    'git push origin HEAD:main',
    'git push origin fatia/0000-x:refs/heads/main',
    'git push origin master',
    'git push --all origin',
    'git push --mirror origin',
    'git push origin --delete fatia/0000-x',
    'git push origin :fatia/0000-x',
    'git push --no-verify',
    'git commit --no-verify -m "x"',
    'git commit -n -m "x"',
    'git -c core.hooksPath=/dev/null commit -m "x"',
    'git commit --no-gpg-sign -m "x"',
    'git config user.email x@example.com',
    'git config --global user.name x',
    'git config core.hooksPath other',
    'git rebase main',
    'git commit --amend -m "x"',
    'git reset --hard HEAD~1',
    'git branch -D fatia/0000-x',
    'git branch --delete fatia/0000-x',
    'git clean -fdx',
    'git merge fatia/0000-x',
    'git merge --squash fatia/0000-x',
    'git merge --no-ff fatia/0000-x',
    'git switch main && git merge fatia/0000-x',
    'gh auth login',
    'gh auth switch',
    'gh api repos/owner/repo/rulesets',
    'gh pr merge 1 --squash',
    'gh pr review 1 --approve',
    'gh repo edit --visibility private',
    'GH_TOKEN=abc gh pr list',
    'export GIT_DIR=/tmp/other',
    '$env:GH_TOKEN = "x"',
    'env -u GH_TOKEN gh pr list',
    'cat ~/.ssh/id_ed25519',
    'echo x > .claude/settings.json',
    "sed -i 's/a/b/' .githooks/pre-commit",
    'rm -rf .github',
    'Set-Content -Path AGENTS.md -Value x',
    'cp other.json vcpkg.json',
    'cp other.json vcpkg-configuration.json',
    'echo x > CMakePresets.json',
    "sed -i 's/a/b/' cmake/NexumModule.cmake",
    'git checkout -- .clang-tidy',
    'echo x >> .clang-format',
    'rm scripts/dev.py',
    'rm -rf scripts/checks',
    'echo bmd >> scripts/checks/forbidden_terms.txt',
    'rm -rf tests/architecture',
    'echo x >> docs/ROADMAP.md',
    'echo x >> docs/VISION.md',
    'echo x >> docs/VISUAL.md',
    'cp new.md docs/referencia/especificacao-engine-propria-caminho-b.md',
    'Copy-Item new.md docs\referencia\especificacao-engine-propria-caminho-b.md',
    'cp new.png tests/visual/approved/scene.png',
    'rm -rf tests/visual/approved',
    'mv tests/hooks/guard-bash.tests.ps1 /tmp/'
)

$allowed = @(
    'git status --short',
    'git push',
    'git push -u origin fatia/0000-fundacao',
    'git push origin fatia/0000-main-loop',
    'git commit -q -m "feat(0000): T01 add the tick clock"',
    'git commit -m "docs(prep): update AGENTS.md" -m "Co-Authored-By: Claude <noreply@anthropic.com>"',
    'git add .claude/skills/situacao/SKILL.md',
    'git config --get user.name',
    'git log --oneline -5',
    'git diff main...HEAD',
    'git merge-base main HEAD',
    'git switch -c fatia/0000-fundacao',
    'gh pr create --base main --head fatia/0000-x --title "Fatia 0" --body-file body.md',
    'gh pr view --json state',
    'gh pr checks',
    'py scripts/dev.py verify',
    'py scripts/dev.py verify 2>&1 | tail -5',
    'py scripts/checks/run.py all > /dev/null',
    'cmake --preset windows-dev',
    'cat .claude/settings.json',
    'cat docs/referencia/README.md',
    'grep -r "CA-01" tests/',
    'echo hello > notes.txt'
)

$failures = 0

foreach ($command in $blocked) {
    if (-not (Get-GuardViolation -Command $command -CurrentBranch 'fatia/0000-x')) {
        Write-Output "FAIL (should be blocked): $command"
        $failures++
    }
}

foreach ($command in $allowed) {
    $reason = Get-GuardViolation -Command $command -CurrentBranch 'fatia/0000-x'
    if ($reason) {
        Write-Output "FAIL (should be allowed): $command -> $reason"
        $failures++
    }
}

if (-not (Get-GuardViolation -Command 'git commit -m "x"' -CurrentBranch 'main')) {
    Write-Output 'FAIL (should be blocked): commit while on main'
    $failures++
}

$blockedOnMain = @('git push', 'git push origin', 'git push origin HEAD', 'git push -u origin HEAD', 'git merge fatia/0000-x')
foreach ($command in $blockedOnMain) {
    if (-not (Get-GuardViolation -Command $command -CurrentBranch 'main')) {
        Write-Output "FAIL (should be blocked while on main): $command"
        $failures++
    }
}

# The local mode (docs/WORKFLOW.md): the fast-forward merge, after the owner's yes, does not commit on main.
$allowedOnMain = @('git push -u origin fatia/0000-x', 'git pull --ff-only', 'git switch main', 'git merge --ff-only fatia/0000-x')
foreach ($command in $allowedOnMain) {
    $reason = Get-GuardViolation -Command $command -CurrentBranch 'main'
    if ($reason) {
        Write-Output "FAIL (should be allowed while on main): $command -> $reason"
        $failures++
    }
}

$total = $blocked.Count + $allowed.Count + 1 + $blockedOnMain.Count + $allowedOnMain.Count
Write-Output "guard-bash tests: $($total - $failures) of $total passed."
if ($failures -gt 0) { exit 1 }
exit 0
