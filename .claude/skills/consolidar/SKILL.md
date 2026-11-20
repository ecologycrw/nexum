---
name: consolidar
description: Faxina periódica da documentação do Nexum - contradições, documentos desatualizados, regras duplicadas, desvios da spec de referência, propostas e itens provisórios antigos. Use a cada cerca de 5 fatias ou quando o dono pedir.
---

# /consolidar

## Pré-condições

- Nenhuma fatia em andamento e a árvore de trabalho limpa.

## Passos

1. **Branch.** `git switch main`. Se houver remoto, `git pull --ff-only`. Depois, `git switch -c consolidacao/AAAA-MM-DD`.
2. **Auditoria.** Chame o subagente `auditor` com a ferramenta de agentes e peça a auditoria completa da documentação.
3. **Confira os achados** no texto real. O auditor também pode errar.
4. **Apresente ao dono,** em português simples, a limpeza proposta:
   - o que apagar;
   - o que juntar;
   - o que corrigir;
   - o que transformar em trava.

   Aplique só o que o dono aprovar. Mudanças em travas pedem a confirmação do dono em cada edição.
5. **Verifique.** Rode `/verificar`.
6. **Commit e revisão.** Commit `docs(consolidation): ...`. Com remoto: `git push -u origin consolidacao/AAAA-MM-DD` e PR com `gh pr create`; o dono aprova e faz o merge. Sem remoto: o merge local segue a regra do modo local (`docs/WORKFLOW.md`).
