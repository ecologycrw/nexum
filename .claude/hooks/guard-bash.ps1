# Claude Code PreToolUse hook for the Bash and PowerShell tools.
# Blocks forbidden commands (AGENTS.md, "Proibido") and explains why.
# Exit code 2 blocks the command; the message on stderr is shown to the agent.
# Tests: tests/hooks/guard-bash.tests.ps1. This file is a protected "trava".

function Get-GuardViolation {
    param(
        [Parameter(Mandatory = $true)] [AllowEmptyString()] [string] $Command,
        [string] $CurrentBranch = ''
    )

    $c = ($Command -replace '\s+', ' ').Trim()
    $git = '\bgit\s+(?:-[cC]\s+\S+\s+)*'

    $rules = @(
        @{ Pattern = $git + 'push\b[^;&|]*(\s--force(-with-lease)?\b|\s-[a-zA-Z]*f[a-zA-Z]*\b|\s\+\S)'
           Reason  = 'Force push is forbidden: history is never rewritten. Push normally.' },
        @{ Pattern = $git + 'push\b[^;&|]*[\s:](refs/heads/)?(main|master)(\s|$|[;&|])'
           Reason  = 'Nobody pushes to main or master. Push the slice branch and open a pull request.' },
        @{ Pattern = $git + 'push\b[^;&|]*\s(--all|--mirror|--delete|-d)\b'
           Reason  = 'Pushing all branches, mirroring or deleting remote branches is forbidden.' },
        @{ Pattern = $git + 'push\b[^;&|]*\s:\S'
           Reason  = 'Deleting remote branches is forbidden.' },
        @{ Pattern = '--no-verify\b'
           Reason  = 'Skipping the Git hooks (--no-verify) is forbidden. Fix what the hook reports.' },
        @{ Pattern = $git + 'commit\b[^;&|]*\s-[a-zA-Z]*n[a-zA-Z]*\b'
           Reason  = 'git commit -n skips the Git hooks and is forbidden. Fix what the hook reports.' },
        @{ Pattern = '--no-gpg-sign\b|commit\.gpgsign|core\.hookspath|\bgpg\.format\b|user\.signingkey'
           Reason  = 'Changing the Git hooks or commit signing is forbidden.' },
        @{ Pattern = $git + 'config\b(?![^;&|]*(--get|--list|\s-l\b))'
           Reason  = 'git config is reserved for the owner. To read a value, use: git config --get <key>.' },
        @{ Pattern = $git + '(rebase|filter-branch|filter-repo|update-ref|replace)\b'
           Reason  = 'Rewriting history is forbidden. Make a new commit instead.' },
        @{ Pattern = $git + 'commit\b[^;&|]*--amend\b'
           Reason  = 'git commit --amend rewrites history and is forbidden. Make a new commit instead.' },
        @{ Pattern = $git + 'reset\b[^;&|]*--hard\b'
           Reason  = 'git reset --hard can destroy work and is forbidden. Ask the owner.' },
        @{ Pattern = $git + 'branch\b[^;&|]*\s(-d|--delete)\b'
           Reason  = 'Deleting branches is forbidden.' },
        @{ Pattern = $git + 'clean\b[^;&|]*\s-[a-zA-Z]*f'
           Reason  = 'git clean -f deletes untracked files and is forbidden. Ask the owner.' },
        @{ Pattern = $git + 'merge(?=\s|$)(?![^;&|]*--ff-only\b)'
           Reason  = 'Only a fast-forward merge is allowed (git merge --ff-only), and only after an explicit yes from the owner (docs/WORKFLOW.md, "O merge").' },
        @{ Pattern = '\bgh\s+(auth|api|secret|variable|ruleset)\b'
           Reason  = 'gh auth, gh api, secrets, variables and rulesets are reserved for the owner.' },
        @{ Pattern = '\bgh\s+pr\s+(merge|review)\b'
           Reason  = 'Only the owner reviews and merges pull requests, in the browser.' },
        @{ Pattern = '\bgh\s+repo\s+(edit|delete|rename|archive)\b'
           Reason  = 'Repository settings are reserved for the owner.' },
        @{ Pattern = '\b(GH_TOKEN|GITHUB_TOKEN|GH_ENTERPRISE_TOKEN)\b|\bGIT_[A-Z_]+\s*=|\$env:(GIT_|GH_)|\benv\s+-u\b'
           Reason  = 'Changing Git or GitHub credentials and environment variables is forbidden.' },
        @{ Pattern = '\.ssh[/\\]'
           Reason  = 'Reading or changing SSH keys is forbidden.' }
    )

    foreach ($rule in $rules) {
        if ($c -match $rule.Pattern) {
            return $rule.Reason
        }
    }

    if (($CurrentBranch -eq 'main' -or $CurrentBranch -eq 'master') -and $c -match ($git + 'commit\b')) {
        return 'Committing directly on main is forbidden. Switch to the slice branch (fatia/NNNN-name).'
    }

    # A push without a branch name sends the current branch.
    $pushesCurrentBranch = $git + 'push(\s+-\S+)*(\s+origin)?(\s+HEAD)?\s*($|[;&|])'
    if (($CurrentBranch -eq 'main' -or $CurrentBranch -eq 'master') -and $c -match $pushesCurrentBranch) {
        return 'Nobody pushes to main or master. Switch to the slice branch (fatia/NNNN-name).'
    }

    # Protected files ("travas") must be changed with the editing tools, which ask the owner first.
    $protected = '(\.(claude|githooks|github)([/\\\s"'']|$)|\bAGENTS\.md|\bCLAUDE\.md|\bcmake[/\\]|CMakePresets\.json|vcpkg(-configuration)?\.json|\.clang-(format|tidy)|\.editorconfig|scripts[/\\]dev\.py|scripts[/\\]checks|tests[/\\]architecture|tests[/\\]hooks|tests[/\\]visual[/\\]approved|docs[/\\]referencia|docs[/\\](VISION|ROADMAP|VISUAL)\.md)'
    # Not writes: discarded output, arrows in code, and e-mail addresses such as the Co-Authored-By line.
    $withoutHarmlessRedirects = $c -replace '\d?>&\d|\d?>\s*(/dev/null|\$null|nul)\b|=>|->|<[^<>\s]+@[^<>\s]+>', ' '
    $write = '(>|\bsed\s[^;&|]*-i|\btee\b|\b(rm|mv|cp|truncate|del|rmdir)\s|Set-Content|Add-Content|Out-File|Remove-Item|Move-Item|Copy-Item|New-Item|Rename-Item|' + $git + '(rm|mv|checkout|restore|apply)\b)'
    if ($c -match $protected -and $withoutHarmlessRedirects -match $write) {
        return 'Protected files (travas) cannot be changed from the terminal. Use the editing tools, which ask the owner first, and explain why the change is needed.'
    }

    return $null
}

if ($MyInvocation.InvocationName -ne '.') {
    try {
        $payload = [Console]::In.ReadToEnd().TrimStart([char]0xFEFF) | ConvertFrom-Json
        $command = [string]$payload.tool_input.command
    }
    catch {
        [Console]::Error.WriteLine('Blocked by guard-bash.ps1: the command could not be read, so it was blocked for safety.')
        exit 2
    }

    if ([string]::IsNullOrWhiteSpace($command)) {
        exit 0
    }

    $branch = ''
    try {
        if ($env:CLAUDE_PROJECT_DIR) { Set-Location -LiteralPath $env:CLAUDE_PROJECT_DIR }
        $branch = [string](git rev-parse --abbrev-ref HEAD 2>$null)
    }
    catch {
        $branch = ''
    }

    $reason = Get-GuardViolation -Command $command -CurrentBranch $branch.Trim()
    if ($reason) {
        [Console]::Error.WriteLine("Blocked by .claude/hooks/guard-bash.ps1: $reason")
        exit 2
    }

    exit 0
}
