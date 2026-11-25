---
name: fechar
description: Fecha a fatia do Nexum - escreve o report com a prova de cada critério e a checklist de sala limpa, atualiza a documentação viva e abre o Pull Request (ou tira do rascunho o que já existe) para o dono testar. Use depois do /revisar.
---

# /fechar

## Pré-condições

- Todas as tarefas `[x]`, `/verificar` verde e `/revisar` feito (seção "Revisão" preenchida e etapa Revisão marcada no `tasks.md`).

## Passos

1. **Report.** Escreva `specs/NNNN-nome/report.md` a partir de `templates/report.md`:
   - a tabela de critérios com a prova de cada um;
   - a paridade: o nível tocado, o teste e o resultado;
   - a prova visual, se a fatia muda o que aparece na tela: as imagens para o dono aprovar, a comparação e o tempo de quadro;
   - **a "Sala limpa":** a checklist conferida tarefa a tarefa (a checagem `docs` exige a seção);
   - **"O que você deve testar":** passo a passo para o dono, com comandos prontos e o que deve aparecer;
   - desvios do plano, revisão, itens provisórios e propostas geradas;
   - **"Travas alteradas e testes removidos":** as travas que mudaram (`git diff --name-only main...HEAD`, comparando com a lista de travas do `AGENTS.md`) e os testes removidos ou enfraquecidos, com o motivo (a checagem `docs` exige a seção, com as duas listas).
2. **Documentação viva.**
   - `docs/ARCHITECTURE.md`, se algo do que ele descreve mudou (inclusive a linha "Como é agora").
   - Documentação de módulo para cada sistema novo, a partir de `templates/module.md`.
   - "Código de referência" em `docs/CONVENTIONS.md`, se surgiu um bom exemplo.
   - `docs/TECH_DEBT.md`: tire os itens resolvidos e inclua os novos. Os dois vão para a seção "Itens provisórios" do report, em "Criados" e "Resolvidos".
   - ADR para cada decisão nova do Nexum e para cada desvio da spec de referência, a partir de `templates/adr.md`, com a linha no índice `docs/adr/README.md`. Uma decisão que só segue a spec é citada pelo número (D-xx), sem ADR próprio; a regra completa fica no `docs/adr/README.md`.
3. **Status.** Mude a spec para `concluida`. Atualize o `docs/STATUS.md`: a fatia aguarda o teste do dono, com uma linha no "Histórico". Marque a etapa Fechamento na lista "Etapas" do `tasks.md`, com a data: `` - [x] Fechamento (`/fechar`): AAAA-MM-DD ``.
4. **Verifique.** Rode `/verificar`. A checagem de docs confere que todo critério tem prova e que o report tem a sala limpa.
5. **Commit e revisão do dono.**
   - Commit `docs(NNNN): close slice N`.
   - `git push`. Escreva a descrição do PR num arquivo temporário, em português, com: resumo, o que testar, critérios e provas, sala limpa, travas alteradas. Termine com a linha de atribuição do Claude Code.
   - Confira se a branch já tem um PR: `gh pr view fatia/NNNN-nome --json number,state,isDraft`. Uma fatia pode ter aberto o PR em rascunho antes, por exemplo para testar a CI.
     - **Sem PR:** `gh pr create --base main --head fatia/NNNN-nome --title "Fatia N: <título>" --body-file <arquivo>`.
     - **Com PR aberto:** `gh pr edit <número> --title "Fatia N: <título>" --body-file <arquivo>` e, se ele estiver em rascunho, `gh pr ready <número>`. Nunca abra um segundo PR para a mesma branch.
6. **Avise o dono:**
   - o link do PR e o passo a passo do que testar;
   - que o check verde no PR é a prova de que tudo passou nos 3 sistemas;
   - que, depois do teste, o dono conta o que achou e a `/retrospectiva` roda na mesma branch, antes da aprovação.

## Nunca

- Fazer merge. O merge é do dono, pelo navegador.
