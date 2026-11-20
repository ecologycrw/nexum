---
name: fechar
description: Fecha a fatia do Nexum - escreve o report com a prova de cada critério e a checklist de sala limpa, atualiza a documentação viva e abre o Pull Request (ou prepara a revisão local) para o dono testar. Use depois do /revisar.
---

# /fechar

## Pré-condições

- Todas as tarefas `[x]`, `/verificar` verde e `/revisar` feito (seção "Revisão" preenchida no `tasks.md`).

## Passos

1. **Report.** Escreva `specs/NNNN-nome/report.md` a partir de `templates/report.md`:
   - a tabela de critérios com a prova de cada um;
   - a paridade: o nível tocado, o teste e o resultado;
   - a prova visual, se a fatia muda o que aparece na tela: as imagens para o dono aprovar, a comparação e o tempo de quadro;
   - **a "Sala limpa":** a checklist conferida tarefa a tarefa (a checagem `docs` exige a seção);
   - **"O que você deve testar":** passo a passo para o dono, com comandos prontos e o que deve aparecer;
   - desvios do plano, revisão, itens provisórios e propostas geradas;
   - travas alteradas (`git diff --name-only main...HEAD`, comparando com a lista de travas do `AGENTS.md`) e testes removidos, com o motivo.
2. **Documentação viva.**
   - `docs/ARCHITECTURE.md`, se algo do que ele descreve mudou (inclusive a linha "Como é agora").
   - Documentação de módulo para cada sistema novo, a partir de `templates/module.md`.
   - "Código de referência" em `docs/CONVENTIONS.md`, se surgiu um bom exemplo.
   - `docs/TECH_DEBT.md`: tire os itens resolvidos e inclua os novos.
   - ADR para cada decisão nova, a partir de `templates/adr.md`, com a linha no índice `docs/adr/README.md`.
3. **Status.** Mude a spec para `concluida`. Atualize o `docs/STATUS.md`: a fatia aguarda o teste do dono, com uma linha no "Histórico".
4. **Verifique.** Rode `/verificar`. A checagem de docs confere que todo critério tem prova e que o report tem a sala limpa.
5. **Commit e revisão do dono.**
   - Commit `docs(NNNN): close slice N`.
   - **Com remoto:** `git push`. Escreva a descrição do PR num arquivo temporário, em português, com: resumo, o que testar, critérios e provas, sala limpa, travas alteradas. Termine com a linha de atribuição do Claude Code. Rode `gh pr create --base main --head fatia/NNNN-nome --title "Fatia N: <título>" --body-file <arquivo>`.
   - **Sem remoto (modo local):** mostre ao dono o resumo do que mudou (`git log --oneline main..HEAD` e `git diff --stat main...HEAD`) e o caminho do report.
6. **Avise o dono:**
   - o link do PR (ou o resumo local) e o passo a passo do que testar;
   - que, com o GitHub, o check verde no PR é a prova de que tudo passou nos 3 sistemas; no modo local, a prova é a saída do `verify` desta sessão;
   - que, depois do teste, o dono conta o que achou e a `/retrospectiva` roda na mesma branch, antes da aprovação.

## Nunca

- Fazer merge. No GitHub, o merge é do dono, pelo navegador. No modo local, só com o "sim" explícito do dono, e só `git merge --ff-only`.
