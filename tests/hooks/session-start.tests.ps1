# Tests for the task summary of .claude/hooks/session-start.ps1 (P-0003). Runs locally through py scripts/dev.py pytests.
# This file is a protected "trava".

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\..\.claude\hooks\session-start.ps1')

$script:failures = 0
$script:total = 0

function Assert-Lines([string] $Name, [string[]] $Expected, [string[]] $Actual) {
    $script:total++
    $wanted = $Expected -join "`n"
    $found = $Actual -join "`n"
    if ($wanted -cne $found) {
        Write-Output "FAIL ($Name):`n  expected: $wanted`n  found:    $found"
        $script:failures++
    }
}

$path = 'specs/0007-exemplo/tasks.md'
$header = 'Tarefas da fatia 0007 (specs/0007-exemplo/tasks.md), contadas pelo hook:'

Assert-Lines 'the next task is the first pending one, and the notes below a task are not tasks' @(
    "$header 2 feitas, 0 em andamento, 0 bloqueadas, 2 pendentes.",
    'Proxima tarefa: T03: Faz a terceira coisa.'
) (Format-TaskSummary '0007' $path @(
        '# Tarefas da fatia 7',
        'Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada',
        '- [x] T01: Faz a primeira coisa (CA-01). Verificação: verify.',
        '  - Resultado: verde.',
        '- [x] T02: Faz a segunda coisa.',
        '- [ ] T03: Faz a terceira coisa. Verificação: verify.',
        '- [ ] T04: Faz a quarta coisa.',
        '- [ ] Uma linha sem número de tarefa.'
    ))

Assert-Lines 'a task in progress comes before the pending ones' @(
    "$header 1 feitas, 1 em andamento, 0 bloqueadas, 1 pendentes.",
    'Em andamento: T02: Faz a segunda coisa.'
) (Format-TaskSummary '0007' $path @(
        '- [x] T01: Faz a primeira coisa.',
        '- [~] T02: Faz a segunda coisa. Verificação: verify.',
        '- [ ] T03: Faz a terceira coisa.'
    ))

Assert-Lines 'a blocked task is named, with where to read why' @(
    "$header 1 feitas, 0 em andamento, 1 bloqueadas, 1 pendentes.",
    'Bloqueada: T02: Espera a central. Veja Bloqueios no tasks.md.',
    'Proxima tarefa: T03: Faz a terceira coisa.'
) (Format-TaskSummary '0007' $path @(
        '- [x] T01: Faz a primeira coisa.',
        '- [!] T02: Espera a central. Verificação: a resposta.',
        '- [ ] T03: Faz a terceira coisa.'
    ))

Assert-Lines 'a finished list says so' @(
    "$header 2 feitas, 0 em andamento, 0 bloqueadas, 0 pendentes.",
    'Todas as tarefas estao feitas.'
) (Format-TaskSummary '0007' $path @('- [x] T01: Uma.', '- [x] T02: Duas.'))

Assert-Lines 'an empty list says so' @(
    "$header 0 feitas, 0 em andamento, 0 bloqueadas, 0 pendentes.",
    'O tasks.md ainda nao tem nenhuma tarefa.'
) (Format-TaskSummary '0007' $path @('# Tarefas da fatia 7', ''))

$long = 'T01: ' + ('a' * 200)
Assert-Lines 'a long task without a sentence end is shortened' @(
    "$header 0 feitas, 0 em andamento, 0 bloqueadas, 1 pendentes.",
    ('Proxima tarefa: ' + $long.Substring(0, 157) + '...')
) (Format-TaskSummary '0007' $path @("- [ ] $long"))

Assert-Lines 'the slice comes from the front matter of the STATUS' @('0000') @(
    Get-CurrentSpec @('---', 'fase: em-andamento', 'spec-atual: 0000', 'branch: fatia/0000-fundacao', '---'))

Assert-Lines 'an empty spec-atual means no slice in progress' @('') @(
    Get-CurrentSpec @('---', 'fase: entre-fatias', 'spec-atual:', '---'))

Write-Output "session-start tests: $($script:total - $script:failures) of $($script:total) passed."
if ($script:failures -gt 0) { exit 1 }
exit 0
