---
name: consolidar
description: Faxina periódica da documentação do Nexum - contradições, documentos desatualizados, regras duplicadas, desvios da spec de referência, propostas e itens provisórios antigos. Use a cada cerca de 5 fatias ou quando o dono pedir.
---

# /consolidar

## Pré-condições

- Nenhuma fatia em andamento e a árvore de trabalho limpa. A única exceção são propostas novas em `docs/PROPOSALS.md` registradas para esta consolidação: elas entram no primeiro commit da branch.

## Passos

1. **Branch.** `git switch main` e `git pull --ff-only`. Depois, `git switch -c consolidacao/AAAA-MM-DD`.
2. **Auditoria.** Chame o subagente `auditor` com a ferramenta de agentes e peça a auditoria completa da documentação.
3. **Confira os achados** no texto real. O auditor também pode errar. Confira também os destinos: se algum ADR mudou a ordem dos marcos desde a última consolidação, cada item do `docs/TECH_DEBT.md` e cada proposta, aberta ou aceita com o destino ainda por cumprir, que cite um marco que andou precisa de um destino que ainda faça sentido (P-0013).
4. **Apresente ao dono,** em português simples, a limpeza proposta:
   - o que apagar;
   - o que juntar;
   - o que corrigir;
   - o que transformar em trava.

   Aplique só o que o dono aprovar. Mudanças em travas pedem a confirmação do dono em cada edição.
5. **Propostas tratadas.** Cada proposta que esta consolidação processou vai, em `docs/PROPOSALS.md`, de "Abertas" para o topo de "Processadas", com `Status: aceita` ou `descartada` e uma linha "Destino": o que foi feito com cada item, o commit e o que ficou para depois. Se a auditoria corrigiu alguma evidência da proposta, diga isso no destino; o texto original da proposta não é reescrito.
6. **Verifique.** Rode `/verificar`.
7. **Commit e revisão.** Commit `docs(consolidation): ...`, `git push -u origin consolidacao/AAAA-MM-DD` e PR com `gh pr create`; o dono aprova e faz o squash merge pelo navegador.
