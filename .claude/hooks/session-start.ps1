# Claude Code SessionStart hook: prints where the project is, so every session starts with the same context.
# Whatever this script prints is added to the session context. This file is a protected "trava".
# Tests: tests/hooks/session-start.tests.ps1.

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

# The slice in progress, from the front matter of docs/STATUS.md ("spec-atual: 0000"), or '' when there is none.
function Get-CurrentSpec([string[]] $StatusLines) {
    foreach ($line in $StatusLines) {
        if ($line -match '^spec-atual:\s*(\d{4})\s*$') { return $Matches[1] }
    }
    return ''
}

# The first sentence of a task, which is enough to say what it is.
function Get-TaskTitle([string] $Text) {
    $end = $Text.IndexOf('. ')
    $title = if ($end -ge 0) { $Text.Substring(0, $end + 1) } else { $Text }
    if ($title.Length -gt 160) { $title = $title.Substring(0, 157) + '...' }
    return $title
}

# Counts the tasks of a tasks.md and names the one to work on. The summary is computed every time, so it never
# goes stale like a copy in the STATUS would (P-0003).
function Format-TaskSummary([string] $Spec, [string] $Path, [string[]] $Lines) {
    $tasks = @(foreach ($line in $Lines) {
            if ($line -match '^- \[(?<mark>[ x~!])\] (?<text>T\d+:.*)$') {
                [pscustomobject]@{ Mark = $Matches['mark']; Title = (Get-TaskTitle $Matches['text']) }
            }
        })
    $done = @($tasks | Where-Object { $_.Mark -eq 'x' })
    $working = @($tasks | Where-Object { $_.Mark -eq '~' })
    $blocked = @($tasks | Where-Object { $_.Mark -eq '!' })
    $pending = @($tasks | Where-Object { $_.Mark -eq ' ' })

    $summary = @("Tarefas da fatia $Spec ($Path), contadas pelo hook: $($done.Count) feitas, $($working.Count) em andamento, $($blocked.Count) bloqueadas, $($pending.Count) pendentes.")
    foreach ($task in $blocked) { $summary += "Bloqueada: $($task.Title) Veja Bloqueios no tasks.md." }
    if ($working.Count -gt 0) {
        $summary += "Em andamento: $($working[0].Title)"
    }
    elseif ($pending.Count -gt 0) {
        $summary += "Proxima tarefa: $($pending[0].Title)"
    }
    elseif ($tasks.Count -eq 0) {
        $summary += 'O tasks.md ainda nao tem nenhuma tarefa.'
    }
    elseif ($blocked.Count -eq 0) {
        $summary += 'Todas as tarefas estao feitas.'
    }
    return $summary
}

# The stages after the tasks, from the "## Etapas" list of a tasks.md (P-0006). Each step of the cycle marks its
# own line when it finishes, with the date and the commit. The lines are found by ASCII words, so the script
# reads them right in any console. A stage that is not in the list is $null.
function Get-StageMarks([string[]] $Lines) {
    $marks = [ordered]@{ review = $null; close = $null; ownerTest = $null; retrospective = $null }
    foreach ($line in (Get-Section $Lines '^## Etapas' '^## ')) {
        if ($line -notmatch '^- \[(?<mark>[ x])\] (?<text>.*)$') { continue }
        $done = $Matches['mark'] -eq 'x'
        $text = $Matches['text']
        $commit = ''
        if ($text -match 'commit `?(?<hash>[0-9a-f]{7,40})') { $commit = $Matches['hash'] }
        $key = $null
        if ($text -match '/revisar') { $key = 'review' }
        elseif ($text -match '/fechar') { $key = 'close' }
        elseif ($text -match 'Teste do dono') { $key = 'ownerTest' }
        elseif ($text -match '/retrospectiva') { $key = 'retrospective' }
        if ($key) { $marks[$key] = [pscustomobject]@{ Done = $done; Commit = $commit } }
    }
    return $marks
}

# The stage of the slice cycle (docs/WORKFLOW.md), the next step and who acts, computed from the files every time,
# so it never goes stale (P-0006). $State holds what the caller read:
#   Spec        the spec-atual of the STATUS, or '' when it is empty
#   Branch      the current Git branch
#   StatusBranch the branch the STATUS names, or ''
#   SpecStatus  the status in the front matter of the slice's spec.md, or '' when there is no spec
#   Tasks       the lines of the slice's tasks.md, or $null when it does not exist
#   HasReport   whether the slice's report.md exists
#   ChangedSinceReview  the code files changed since the commit of the review
#   ReviewCommitMissing whether the review names a commit that Git does not know
function Format-NextStep([hashtable] $State) {
    if (-not $State.Spec) {
        if ($State.Branch -match '^fatia/(?<number>\d{4})-') {
            $number = $Matches['number']
            switch ($State.SpecStatus) {
                'rascunho' { return @("Etapa: a spec da fatia $number espera a aprovacao do dono. Proximo passo: o dono aprova a spec, e o /planejar continua do passo 9. Quem age: o dono.") }
                'concluida' { return @("Etapa: a fatia $number esta fechada, com a retrospectiva feita. Proximo passo: o dono aprova o PR e faz o squash merge; depois, a proxima fatia comeca na main com /planejar. Quem age: o dono.") }
                '' { return @("Etapa: o planejamento da fatia $number, ainda sem spec. Proximo passo: /planejar. Quem age: a IA.") }
                default { return @("Atencao: a spec da fatia $number esta '$($State.SpecStatus)', mas o spec-atual do STATUS esta vazio. Avise o dono.") }
            }
        }
        return @('Etapa: entre fatias. Proximo passo: /planejar. Quem age: a IA, e o dono aprova a spec.')
    }

    $lines = @()
    if ($null -eq $State.Tasks) {
        $lines += 'Etapa: a spec aprovada, ainda sem a lista de tarefas. Proximo passo: /implementar, que cria o tasks.md. Quem age: a IA.'
    }
    else {
        $marks = @(foreach ($line in $State.Tasks) { if ($line -match '^- \[(?<mark>[ x~!])\] T\d+:') { $Matches['mark'] } })
        $open = @($marks | Where-Object { $_ -eq ' ' -or $_ -eq '~' }).Count
        $blocked = @($marks | Where-Object { $_ -eq '!' }).Count
        if ($marks.Count -eq 0 -or $open -gt 0) {
            $lines += 'Etapa: implementacao. Proximo passo: /implementar. Quem age: a IA.'
        }
        elseif ($blocked -gt 0) {
            $lines += 'Etapa: implementacao, com tarefa bloqueada. Proximo passo: o dono decide o bloqueio (Bloqueios no tasks.md). Quem age: o dono.'
        }
        else {
            $stages = Get-StageMarks $State.Tasks
            $next = @(
                @{ Key = 'review'; Text = 'Etapa: tarefas concluidas. Proximo passo: /revisar. Quem age: a IA.' },
                @{ Key = 'close'; Text = 'Etapa: revisao feita. Proximo passo: /fechar. Quem age: a IA.' },
                @{ Key = 'ownerTest'; Text = 'Etapa: fatia fechada. Proximo passo: o dono testa o PR (o "O que voce deve testar" do report) e conta o resultado. Quem age: o dono.' },
                @{ Key = 'retrospective'; Text = 'Etapa: teste do dono feito. Proximo passo: /retrospectiva. Quem age: a IA.' }
            ) | Where-Object { $null -eq $stages[$_.Key] -or -not $stages[$_.Key].Done } | Select-Object -First 1
            if ($next) { $lines += $next.Text }
            else { $lines += 'Etapa: retrospectiva feita. Proximo passo: o dono aprova o PR e faz o squash merge. Quem age: o dono.' }

            if (@($stages.Values | Where-Object { $null -ne $_ }).Count -eq 0) {
                $lines += 'Atencao: o tasks.md nao tem a lista "## Etapas" (ver templates/tasks.md). Cada etapa marca a sua linha nela ao terminar.'
            }
            if ($stages['close'] -and $stages['close'].Done -and -not $State.HasReport) {
                $lines += 'Atencao: a etapa Fechamento esta marcada, mas o report.md da fatia nao existe.'
            }
            if ($stages['review'] -and $stages['review'].Done) {
                if ($State.ReviewCommitMissing) {
                    $lines += "Atencao: a etapa Revisao cita o commit $($stages['review'].Commit), que o Git nao conhece."
                }
                elseif (@($State.ChangedSinceReview).Count -gt 0) {
                    $count = @($State.ChangedSinceReview).Count
                    $lines += "Atencao: o codigo mudou depois da revisao (commit $($stages['review'].Commit)): $count arquivo(s), como $(@($State.ChangedSinceReview)[0]). Confira se a revisao precisa ser refeita."
                }
            }
        }
    }
    if ($State.StatusBranch -and $State.Branch -and $State.StatusBranch -ne $State.Branch) {
        $lines += "Atencao: a branch atual ($($State.Branch)) nao e a da fatia no STATUS ($($State.StatusBranch))."
    }
    return $lines
}

# The value of one field of a front matter, such as "status: rascunho", or ''.
function Get-FrontMatterValue([string[]] $Lines, [string] $Field) {
    foreach ($line in $Lines) {
        if ($line -match "^${Field}:\s*(?<value>[^\s#]*)") { return $Matches['value'] }
    }
    return ''
}

# Reads what Format-NextStep needs: the STATUS, the slice's files and, for the review, Git.
function Get-NextStep([string[]] $StatusLines, [string] $Branch) {
    $spec = Get-CurrentSpec $StatusLines
    $number = $spec
    if (-not $number -and $Branch -match '^fatia/(?<number>\d{4})-') { $number = $Matches['number'] }
    $state = @{
        Spec = $spec; Branch = $Branch; StatusBranch = ''; SpecStatus = ''; Tasks = $null; HasReport = $false
        ChangedSinceReview = @(); ReviewCommitMissing = $false
    }
    if ($spec) { $state.StatusBranch = Get-FrontMatterValue $StatusLines 'branch' }
    $folder = $null
    if ($number) {
        $folder = @(Get-ChildItem -LiteralPath 'specs' -Directory -Filter "$number-*" -ErrorAction SilentlyContinue) | Select-Object -First 1
    }
    if ($folder) {
        $base = "specs/$($folder.Name)"
        if (Test-Path -LiteralPath "$base/spec.md") {
            $state.SpecStatus = Get-FrontMatterValue (Get-Content -LiteralPath "$base/spec.md" -Encoding UTF8 -TotalCount 12) 'status'
        }
        if (Test-Path -LiteralPath "$base/tasks.md") { $state.Tasks = @(Get-Content -LiteralPath "$base/tasks.md" -Encoding UTF8) }
        $state.HasReport = Test-Path -LiteralPath "$base/report.md"
    }
    if ($null -ne $state.Tasks) {
        $review = (Get-StageMarks $state.Tasks)['review']
        if ($review -and $review.Done -and $review.Commit) {
            git cat-file -e "$($review.Commit)^{commit}" 2>$null
            if ($LASTEXITCODE -ne 0) { $state.ReviewCommitMissing = $true }
            else {
                # Only code counts: the docs, the specs and the Markdown files change after a review on purpose.
                $state.ChangedSinceReview = @(git diff --name-only $review.Commit HEAD 2>$null |
                        Where-Object { $_ -and $_ -notmatch '^(docs|specs)/' -and $_ -notmatch '\.md$' })
            }
        }
    }
    return Format-NextStep $state
}

# Reads the tasks.md of the slice named by the STATUS.
function Get-TaskSummary([string[]] $StatusLines) {
    $spec = Get-CurrentSpec $StatusLines
    if (-not $spec) { return @('Tarefas: nenhuma fatia em andamento (spec-atual vazio no STATUS).') }
    $folder = @(Get-ChildItem -LiteralPath 'specs' -Directory -Filter "$spec-*" -ErrorAction SilentlyContinue) | Select-Object -First 1
    if (-not $folder) { return @("Tarefas: a pasta specs/$spec-* nao existe. Avise o dono.") }
    $path = "specs/$($folder.Name)/tasks.md"
    if (-not (Test-Path -LiteralPath $path)) { return @("Tarefas: a fatia $spec ainda nao tem $path (o /implementar cria).") }
    return Format-TaskSummary $spec $path (Get-Content -LiteralPath $path -Encoding UTF8)
}

if ($MyInvocation.InvocationName -ne '.') {
    [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
    if ($env:CLAUDE_PROJECT_DIR) { Set-Location -LiteralPath $env:CLAUDE_PROJECT_DIR }

    $branch = git rev-parse --abbrev-ref HEAD 2>$null
    $log = git log --oneline -5 2>$null
    $remote = git remote 2>$null

    $statusLines = @()
    if (Test-Path docs/STATUS.md) { $statusLines = Get-Content docs/STATUS.md -Encoding UTF8 }
    $status = Get-Section $statusLines '^## Agora' '^## Bloqueios'

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
        Write-Output 'Atencao: o repositorio esta sem remoto, mas o Nexum vive no GitHub (docs/setup/GITHUB.md). Avise o dono antes de seguir.'
    }
    Write-Output 'Ultimos commits:'
    $log | ForEach-Object { Write-Output "  $_" }
    Write-Output ''
    Write-Output 'Trecho de docs/STATUS.md:'
    $status | ForEach-Object { Write-Output "  $_" }
    Write-Output ''
    Get-TaskSummary $statusLines | ForEach-Object { Write-Output $_ }
    Get-NextStep $statusLines $branch | ForEach-Object { Write-Output $_ }
    Write-Output "Propostas abertas em docs/PROPOSALS.md: $openProposals"
    Write-Output (Get-ReferenceState)
    Write-Output 'Regras: AGENTS.md (carregado pelo CLAUDE.md). Sala limpa: o codigo do MuMain nunca e lido. Para orientar o dono, use /situacao.'
    exit 0
}
