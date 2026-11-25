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

# The stage of the cycle and the next step (P-0006).

function New-State([hashtable] $Values) {
    $state = @{
        Spec = '0007'; Branch = 'fatia/0007-exemplo'; StatusBranch = 'fatia/0007-exemplo'; SpecStatus = 'em-andamento'
        Tasks = @(); HasReport = $false; ChangedSinceReview = @(); ReviewCommitMissing = $false
    }
    foreach ($key in $Values.Keys) { $state[$key] = $Values[$key] }
    return $state
}

$finished = @('- [x] T01: Uma.', '- [x] T02: Duas.', '', '## Etapas', '')
function Get-FinishedTasks([string[]] $Stages) { return @($finished + $Stages + @('', '## Bloqueios')) }

Assert-Lines 'between slices, on the main branch, the next step is to plan' @(
    'Etapa: entre fatias. Proximo passo: /planejar. Quem age: a IA, e o dono aprova a spec.'
) (Format-NextStep (New-State @{ Spec = ''; Branch = 'main'; StatusBranch = ''; SpecStatus = ''; Tasks = $null }))

Assert-Lines 'a draft spec on its branch waits for the owner' @(
    'Etapa: a spec da fatia 0007 espera a aprovacao do dono. Proximo passo: o dono aprova a spec, e o /planejar continua do passo 9. Quem age: o dono.'
) (Format-NextStep (New-State @{ Spec = ''; StatusBranch = ''; SpecStatus = 'rascunho'; Tasks = $null }))

Assert-Lines 'a slice branch without a spec is still being planned' @(
    'Etapa: o planejamento da fatia 0007, ainda sem spec. Proximo passo: /planejar. Quem age: a IA.'
) (Format-NextStep (New-State @{ Spec = ''; StatusBranch = ''; SpecStatus = ''; Tasks = $null }))

Assert-Lines 'after the retrospective, the slice waits for the merge by the owner' @(
    'Etapa: a fatia 0007 esta fechada, com a retrospectiva feita. Proximo passo: o dono aprova o PR e faz o squash merge; depois, a proxima fatia comeca na main com /planejar. Quem age: o dono.'
) (Format-NextStep (New-State @{ Spec = ''; StatusBranch = ''; SpecStatus = 'concluida'; Tasks = $null }))

Assert-Lines 'a spec in progress with an empty spec-atual is reported' @(
    "Atencao: a spec da fatia 0007 esta 'em-andamento', mas o spec-atual do STATUS esta vazio. Avise o dono."
) (Format-NextStep (New-State @{ Spec = ''; StatusBranch = ''; Tasks = $null }))

Assert-Lines 'an approved spec without a task list goes to /implementar' @(
    'Etapa: a spec aprovada, ainda sem a lista de tarefas. Proximo passo: /implementar, que cria o tasks.md. Quem age: a IA.'
) (Format-NextStep (New-State @{ SpecStatus = 'aprovada'; Tasks = $null }))

Assert-Lines 'pending tasks go to /implementar, even with a blocked one' @(
    'Etapa: implementacao. Proximo passo: /implementar. Quem age: a IA.'
) (Format-NextStep (New-State @{ Tasks = @('- [x] T01: Uma.', '- [!] T02: Duas.', '- [ ] T03: Tres.') }))

Assert-Lines 'when only blocked tasks are left, the owner decides' @(
    'Etapa: implementacao, com tarefa bloqueada. Proximo passo: o dono decide o bloqueio (Bloqueios no tasks.md). Quem age: o dono.'
) (Format-NextStep (New-State @{ Tasks = @('- [x] T01: Uma.', '- [!] T02: Duas.') }))

Assert-Lines 'finished tasks go to /revisar' @(
    'Etapa: tarefas concluidas. Proximo passo: /revisar. Quem age: a IA.'
) (Format-NextStep (New-State @{ Tasks = (Get-FinishedTasks @(
                '- [ ] Revisão (`/revisar`)', '- [ ] Fechamento (`/fechar`)', '- [ ] Teste do dono', '- [ ] Retrospectiva (`/retrospectiva`)')) }))

Assert-Lines 'a task list without the stages still goes to /revisar, and says the list is missing' @(
    'Etapa: tarefas concluidas. Proximo passo: /revisar. Quem age: a IA.',
    'Atencao: o tasks.md nao tem a lista "## Etapas" (ver templates/tasks.md). Cada etapa marca a sua linha nela ao terminar.'
) (Format-NextStep (New-State @{ Tasks = @('- [x] T01: Uma.', '- [x] T02: Duas.') }))

Assert-Lines 'after the review comes /fechar' @(
    'Etapa: revisao feita. Proximo passo: /fechar. Quem age: a IA.'
) (Format-NextStep (New-State @{ Tasks = (Get-FinishedTasks @(
                '- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [ ] Fechamento (`/fechar`)', '- [ ] Teste do dono', '- [ ] Retrospectiva (`/retrospectiva`)')) }))

Assert-Lines 'after the closing, the owner tests' @(
    'Etapa: fatia fechada. Proximo passo: o dono testa o PR (o "O que voce deve testar" do report) e conta o resultado. Quem age: o dono.'
) (Format-NextStep (New-State @{ HasReport = $true; SpecStatus = 'concluida'; Tasks = (Get-FinishedTasks @(
                '- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [x] Fechamento (`/fechar`): 2026-11-22', '- [ ] Teste do dono', '- [ ] Retrospectiva (`/retrospectiva`)')) }))

Assert-Lines 'after the test of the owner comes /retrospectiva' @(
    'Etapa: teste do dono feito. Proximo passo: /retrospectiva. Quem age: a IA.'
) (Format-NextStep (New-State @{ HasReport = $true; Tasks = (Get-FinishedTasks @(
                '- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [x] Fechamento (`/fechar`): 2026-11-22', '- [x] Teste do dono: 2026-11-23, tudo certo', '- [ ] Retrospectiva (`/retrospectiva`)')) }))

Assert-Lines 'a closing without the report is reported' @(
    'Etapa: fatia fechada. Proximo passo: o dono testa o PR (o "O que voce deve testar" do report) e conta o resultado. Quem age: o dono.',
    'Atencao: a etapa Fechamento esta marcada, mas o report.md da fatia nao existe.'
) (Format-NextStep (New-State @{ Tasks = (Get-FinishedTasks @(
                '- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [x] Fechamento (`/fechar`): 2026-11-22', '- [ ] Teste do dono')) }))

Assert-Lines 'code changed after the review is reported' @(
    'Etapa: revisao feita. Proximo passo: /fechar. Quem age: a IA.',
    'Atencao: o codigo mudou depois da revisao (commit abc1234): 2 arquivo(s), como Engine/Core/Private/Version.cpp. Confira se a revisao precisa ser refeita.'
) (Format-NextStep (New-State @{ ChangedSinceReview = @('Engine/Core/Private/Version.cpp', 'Engine/Core/CMakeLists.txt'); Tasks = (Get-FinishedTasks @(
                '- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [ ] Fechamento (`/fechar`)')) }))

Assert-Lines 'a review commit unknown to Git is reported' @(
    'Etapa: revisao feita. Proximo passo: /fechar. Quem age: a IA.',
    'Atencao: a etapa Revisao cita o commit abc1234, que o Git nao conhece.'
) (Format-NextStep (New-State @{ ReviewCommitMissing = $true; Tasks = (Get-FinishedTasks @(
                '- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [ ] Fechamento (`/fechar`)')) }))

Assert-Lines 'a branch other than the one of the STATUS is reported' @(
    'Etapa: implementacao. Proximo passo: /implementar. Quem age: a IA.',
    'Atencao: a branch atual (main) nao e a da fatia no STATUS (fatia/0007-exemplo).'
) (Format-NextStep (New-State @{ Branch = 'main'; Tasks = @('- [ ] T01: Uma.') }))

$stages = Get-StageMarks (Get-FinishedTasks @('- [x] Revisão (`/revisar`): 2026-11-22, commit `abc1234`', '- [ ] Teste do dono'))
Assert-Lines 'the stages keep the commit of the review and leave out the ones not listed' @(
    'review True abc1234', 'close missing', 'ownerTest False', 'retrospective missing'
) @(foreach ($key in $stages.Keys) {
        $stage = $stages[$key]
        if ($null -eq $stage) { "$key missing" } else { "$key $($stage.Done) $($stage.Commit)".TrimEnd() }
    })

Assert-Lines 'a field of a front matter, without its comment' @('rascunho', '') @(
    (Get-FrontMatterValue @('---', 'status: rascunho   # rascunho | aprovada', '---') 'status'),
    (Get-FrontMatterValue @('---', 'id: 0007', '---') 'status'))

Write-Output "session-start tests: $($script:total - $script:failures) of $($script:total) passed."
if ($script:failures -gt 0) { exit 1 }
exit 0
