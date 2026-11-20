---
name: situacao
description: Mostra onde o projeto Nexum está e qual é a próxima ação, em português simples para o dono do produto. Use quando o dono perguntar "onde estamos", "o que falta", "qual o próximo passo" ou ao retomar o trabalho.
---

# /situacao

Responde "onde estamos?" sem mudar nenhum arquivo. (O nome não é `/status` porque esse já é um comando interno do Claude Code.)

## Passos

1. Leia `docs/STATUS.md`, as seções "Agora" e "Próximas" de `docs/ROADMAP.md` (com o marco atual e o que ele manda investigar antes) e conte as propostas abertas em `docs/PROPOSALS.md`.
2. Se o STATUS tiver `spec-atual`, leia `specs/NNNN-*/spec.md` e `tasks.md` (quantas tarefas feitas, em andamento, bloqueadas).
3. Rode `git status --short`, `git log --oneline -5` e `git remote`. Se houver remoto e a branch atual tiver PR aberto, rode `gh pr view --json number,state,url,statusCheckRollup`. Sem remoto, o projeto está no modo local (`docs/WORKFLOW.md`).
4. Responda ao dono em português simples, usando os termos de `docs/GLOSSARY.md`:
   - **Onde estamos:** uma ou duas frases, com o marco (M0, M1...) e a fatia.
   - **O que foi feito desde o último marco.**
   - **O que está esperando por você:** aprovações, testes, decisões, perguntas para levar à central.
   - **Próxima ação recomendada:** qual comando (`/planejar`, `/implementar`, `/fechar`...) e por quê.
5. Se algo estiver inconsistente (STATUS diferente do que o Git mostra, tarefas marcadas mas sem commit, o hook do início da sessão avisando que a spec de referência mudou na central), diga isso claramente.
