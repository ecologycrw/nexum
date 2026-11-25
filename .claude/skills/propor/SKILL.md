---
name: propor
description: Registra uma proposta (ideia, problema, regra que falta, skill ou agente novo, bug fora do escopo) na fila docs/PROPOSALS.md do Nexum, sem implementar. Use quando algo surgir fora da tarefa atual, ou quando o dono pedir algo novo.
---

# /propor

Registra; **não implementa.**

## Passos

1. **Número.** O próximo `P-NNNN` livre, contando as propostas abertas e as processadas.
2. **Entrada.** Na seção "Abertas" de `docs/PROPOSALS.md`, no formato de `templates/proposal.md`:
   - título curto;
   - **Origem:** fatia e tarefa, feedback do dono, medição ou checagem;
   - **Tipo:** fatia, regra, trava, skill, agente, doc, decisão ou bug;
   - **Evidência:** concreta. Se veio do dono, use as palavras do dono;
   - **Prioridade sugerida;**
   - **Status:** `aberta`.
3. **Checagem.** Rode `py scripts/checks/run.py docs terms`.
4. **Commit.**
   - Com uma fatia em andamento: commit na branch dela, `docs(NNNN): add proposal P-NNNN`, e `git push`.
   - Sem fatia em andamento: deixe a mudança só em `docs/PROPOSALS.md`, sem commit. O `/planejar` e o `/consolidar` aceitam essa exceção e levam a proposta no primeiro commit da branch deles. Avise o dono.
   - Numa branch `consolidacao/` aberta: commit na branch dela, `docs(consolidation): add proposal P-NNNN`, e `git push`.
5. **Confirme ao dono** em uma linha: o número da proposta e quando ela será analisada (na próxima `/retrospectiva`, ou no `/consolidar`, se o dono pediu para processá-la lá).
