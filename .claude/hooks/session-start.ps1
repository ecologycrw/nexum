# Claude Code SessionStart hook: prints where the project is, so every session starts with the same context.
# Whatever this script prints is added to the session context. This file is a protected "trava".

[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
if ($env:CLAUDE_PROJECT_DIR) { Set-Location -LiteralPath $env:CLAUDE_PROJECT_DIR }

$ReferenceCopy = 'docs/referencia/especificacao-engine-propria-caminho-b.md'
$ReferenceInCentral = 'docs/specs/arquitetura/especificacao-engine-propria-caminho-b.md'

function Get-Section([string[]] $Lines, [string] $StartPattern, [string] $EndPattern) {
    $start = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        if ($start -lt 0 -and $Lines[$i] -match $StartPattern) { $start = $i; continue }
        if ($start -ge 0 -and $Lines[$i] -match $EndPattern) { return $Lines[$start..($i - 1)] }
    }
    if ($start -ge 0) { return $Lines[$start..($Lines.Count - 1)] }
    return @()
}

# The SHA-256 of the text with LF line endings, the same way docs/referencia/README.md records it.
function Get-NormalizedHash([string] $Path) {
    $text = [System.IO.File]::ReadAllText($Path, [System.Text.Encoding]::UTF8) -replace "`r`n", "`n"
    $bytes = (New-Object System.Text.UTF8Encoding($false)).GetBytes($text)
    $sha = [System.Security.Cryptography.SHA256]::Create()
    return ([System.BitConverter]::ToString($sha.ComputeHash($bytes)) -replace '-', '').ToLowerInvariant()
}

# Whether the copy of the reference spec is the same as the spec in the knowledge center.
function Get-ReferenceState {
    $configuration = '.claude/central.local.json'
    if (-not (Test-Path -LiteralPath $configuration)) {
        return 'Central de conhecimento: nao configurada nesta maquina (.claude/central.local.json).'
    }
    try {
        $central = [string](Get-Content -LiteralPath $configuration -Raw -Encoding UTF8 | ConvertFrom-Json).path
    }
    catch {
        return 'Central de conhecimento: .claude/central.local.json invalido.'
    }
    $source = Join-Path $central $ReferenceInCentral
    if (-not (Test-Path -LiteralPath $source) -or -not (Test-Path -LiteralPath $ReferenceCopy)) {
        return 'Spec de referencia: nao encontrada na central ou no projeto. Avise o dono.'
    }
    if ((Get-NormalizedHash $source) -eq (Get-NormalizedHash $ReferenceCopy)) {
        return 'Spec de referencia: igual a versao da central.'
    }
    return 'ATENCAO: a spec de referencia mudou na central desde a copia em docs/referencia/. Avise o dono e registre com /propor a atualizacao da copia (trava).'
}

$branch = git rev-parse --abbrev-ref HEAD 2>$null
$log = git log --oneline -5 2>$null
$remote = git remote 2>$null

$status = @()
if (Test-Path docs/STATUS.md) {
    $status = Get-Section (Get-Content docs/STATUS.md -Encoding UTF8) '^## Agora' '^## Bloqueios'
}

$openProposals = 0
if (Test-Path docs/PROPOSALS.md) {
    $open = Get-Section (Get-Content docs/PROPOSALS.md -Encoding UTF8) '^## Abertas' '^## Processadas'
    $openProposals = @($open | Where-Object { $_ -match '^### P-\d{4}' }).Count
}

Write-Output 'Nexum - contexto do inicio da sessao (hook session-start)'
Write-Output "Branch atual: $branch"
if ($remote) {
    Write-Output 'Repositorio: publicado (com remoto).'
}
else {
    Write-Output 'Repositorio: modo local, sem remoto (docs/WORKFLOW.md, "Modo local").'
}
Write-Output 'Ultimos commits:'
$log | ForEach-Object { Write-Output "  $_" }
Write-Output ''
Write-Output 'Trecho de docs/STATUS.md:'
$status | ForEach-Object { Write-Output "  $_" }
Write-Output ''
Write-Output "Propostas abertas em docs/PROPOSALS.md: $openProposals"
Write-Output (Get-ReferenceState)
Write-Output 'Regras: AGENTS.md (carregado pelo CLAUDE.md). Sala limpa: o codigo do MuMain nunca e lido. Para orientar o dono, use /situacao.'
exit 0
