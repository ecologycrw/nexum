---
name: retrospectiva
description: Processa o que a fatia do Nexum ensinou - aplica a escada de erros, organiza as propostas e recomenda a próxima fatia ou marco. Roda na mesma branch da fatia, depois que o dono testou e antes de aprovar o merge.
---

# /retrospectiva

## Pré-condições

- A fatia está fechada (PR aberto) e o dono já testou.
- A branch atual é a da fatia.

## Passos

1. **Feedback do dono.** Se houve problemas ou pedidos no teste, registre cada um com `/propor`, usando as palavras do dono. Um bug que impede a fatia de cumprir um critério de aceitação não é proposta: corrija na própria fatia (`/implementar`, `/verificar`) e atualize o report. Se a etapa Teste do dono ainda não estiver marcada na lista "Etapas" do `tasks.md`, marque com a data e uma linha sobre o que o dono achou.
2. **Escada de erros.** Leia "Erros e lições" deste report e dos reports das fatias anteriores. Um erro que apareceu pela segunda vez vira proposta de regra; se puder ser verificado por máquina, a proposta é de teste, hook ou análise automática.
3. **Organize as propostas abertas.** Para cada uma, recomende: fatia nova, ADR, regra, trava, skill, agente ou descartar, com o motivo.
4. **Próxima fatia.** Proponha de 1 a 3 opções, com uma recomendação. Pese:
   1. a ordem dos marcos no ROADMAP;
   2. as dependências e o que o marco manda investigar antes na central;
   3. o risco, porque o mais arriscado vem primeiro (a rede, por exemplo);
   4. o que o dono pediu.
5. **Pergunte ao dono:** qual a próxima fatia e o que fazer com cada proposta. Apresente em português simples.
6. **Depois das respostas:**
   - `docs/ROADMAP.md`: ajuste as fatias e os horizontes (o Claude Code vai pedir a confirmação do dono);
   - se a ordem dos marcos mudou: confira o destino de cada item do `docs/TECH_DEBT.md` e de cada proposta aberta que cite um marco que andou, e corrija o que não fizer mais sentido (P-0013);
   - `docs/PROPOSALS.md`: mova as propostas decididas para "Processadas", com o destino;
   - `docs/STATUS.md`: `spec-atual` vazio, fase `entre-fatias`, próxima ação `/planejar`. Com o `spec-atual` vazio na branch da fatia, o hook do início da sessão passa a dizer que a fatia espera o merge do dono;
   - `tasks.md`: marque a etapa Retrospectiva na lista "Etapas", com a data;
   - imagens novas da prova visual: em geral, o dono já as aprovou durante o `/implementar`, porque o teste da CI precisa delas antes do `/fechar`. Se o teste do dono pediu uma imagem nova, entregue a ele o comando pronto para copiá-la para `tests/visual/approved/`. A IA nunca copia nem altera essas imagens. Depois que o dono rodar o comando, inclua a imagem no commit;
   - mudanças em travas que o dono aprovou: aplique (cada edição pede a confirmação do dono).
7. **Verifique e faça o commit.** Rode `/verificar`. Se passar, faça o commit `docs(NNNN): retrospective of slice N` e o `git push`.
8. **Avise o dono** que a fatia está pronta para a aprovação e o squash merge pelo navegador.
9. **A cada cerca de 5 fatias,** sugira um `/consolidar` depois do merge.
