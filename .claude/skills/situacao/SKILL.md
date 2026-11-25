---
name: situacao
description: Mostra onde o projeto Nexum está e qual é a próxima ação, em português simples para o dono do produto. Use quando o dono perguntar "onde estamos", "o que falta", "qual o próximo passo" ou ao retomar o trabalho.
---

# /situacao

Responde "onde estamos?" sem mudar nenhum arquivo. (O nome não é `/status` porque esse já é um comando interno do Claude Code.)

## Passos

1. Leia `docs/STATUS.md`, as seções "Agora" e "Próximas" de `docs/ROADMAP.md` (com o marco atual e o que ele manda investigar antes) e conte as propostas abertas em `docs/PROPOSALS.md`.
2. **A etapa e o próximo passo vêm do hook do início da sessão,** não de uma dedução: ele lê o STATUS, a spec, as tarefas e a lista "Etapas" do `tasks.md`, e escreve a linha "Etapa: ... Proximo passo: ... Quem age: ...", com avisos ("Atencao") quando algo não bate. No meio da sessão, rode o hook de novo para ter a linha atual: `powershell -NoProfile -ExecutionPolicy Bypass -File .claude/hooks/session-start.ps1` (no Linux e no macOS, `pwsh` no lugar de `powershell`). Se o STATUS tiver `spec-atual`, leia também `specs/NNNN-*/spec.md` e `tasks.md` para contar o que foi feito.
3. Rode `git status --short` e `git log --oneline -5`. Se a branch atual tiver PR aberto, rode `gh pr view --json number,state,isDraft,url,statusCheckRollup`. Diga se o PR está em rascunho, e lembre que o aviso automático da CI (Auto-fix) vale só para a sessão que o ligou: numa sessão nova, ofereça ligar de novo.
4. Responda ao dono em português simples, usando os termos de `docs/GLOSSARY.md`:
   - **Onde estamos:** uma ou duas frases, com o marco (M0, M1...) e a fatia.
   - **O que foi feito desde o último marco.**
   - **O que está esperando por você:** aprovações, testes, decisões, perguntas para levar à central.
   - **Próxima ação recomendada:** o próximo passo da linha "Etapa" do hook, com quem age e por quê.
5. Se algo estiver inconsistente (um "Atencao" do hook, STATUS diferente do que o Git ou o `tasks.md` mostram, tarefas marcadas mas sem commit, o hook avisando que a spec de referência mudou na central), diga isso claramente.
