---
name: implementar
description: Implementa a fatia aprovada do Nexum, uma tarefa por vez, com teste, verificação, checklist de sala limpa e commit a cada tarefa. Use depois que o dono aprovou a spec, ou para continuar uma fatia em andamento.
---

# /implementar

## Pré-condições

- A spec da fatia atual está `aprovada` ou `em-andamento`.
- A branch atual é `fatia/NNNN-nome`.

## Passos

1. **Lista de tarefas.** Se `tasks.md` não existe, crie a partir de `templates/tasks.md`. Divida o plano em tarefas pequenas (T01, T02...), cada uma com o critério que atende e como verificar. Mude a spec para `em-andamento`. Commit `docs(NNNN): add task list`.
   - **O `tasks.md` é curto,** porque toda sessão nova o lê inteiro. Cada tarefa tem a primeira linha do modelo e, no máximo, mais 2 linhas: o resultado em uma frase e o commit. O detalhe (o que foi testado, os números, o que se aprendeu) vai para a mensagem do commit; as provas, os desvios do plano e as travas alteradas vão para o report.
   - **Nunca encurte o que é memória:** as decisões do dono vão para "Decisões do dono", com as palavras dele e a tarefa; os bloqueios, para "Bloqueios"; e o contexto que a próxima sessão precisa, para "Notas para a próxima sessão".
   - **O PR em rascunho** (P-0019). Depois desse commit, faça o `git push` e confira se a branch já tem PR (`gh pr view fatia/NNNN-nome --json number,state,isDraft`). Sem PR, abra um em rascunho: `gh pr create --draft --base main --head fatia/NNNN-nome --title "Fatia N: <título>" --body-file <arquivo>`, com uma descrição curta em português (o que a fatia entrega, que ela está em andamento e o link da spec), terminada com a linha de atribuição do Claude Code. Ofereça ao dono ligar o aviso automático da CI (Auto-fix). A CI só roda quando há PR: assim, cada push passa pelos 3 sistemas, e um erro que só aparece no Linux ou no macOS aparece na própria tarefa. Nunca abra um segundo PR para a mesma branch; o `/fechar` atualiza a descrição e tira o PR do rascunho.
2. **Uma tarefa por vez,** sempre a próxima pendente:
   1. Marque `[~]` no `tasks.md`.
   2. Escreva o teste primeiro, quando possível. O teste que prova um critério leva a etiqueta no começo do nome: `TEST_CASE("[NNNN-CA-xx] ...")`. Os exemplos esperados da seção "Suficiência das descrições" do `plan.md` viram testes. Um teste de recusa confere o motivo exato, e os testes com risco real de passar pelo motivo errado são sabotados uma vez, como diz `docs/CONVENTIONS.md` ("Testes"); a sabotagem vai no corpo do commit (P-0020).
   3. Implemente seguindo `docs/CONVENTIONS.md` e o código de referência citado lá. Respeite as invariantes de `docs/ARCHITECTURE.md`.
   4. **Sala limpa.** Se a tarefa reproduz um comportamento do original, escreva a partir da spec ou do achado listado em "Fontes da central usadas" do `plan.md`, nunca de memória e nunca do código do MuMain. Toda constante (tamanho, offset, tempo, código de pacote) vem de uma spec que cita a origem. Confira a checklist de `docs/WORKFLOW.md` (seção "Sala limpa"). Se a descrição não bastar, pare: use `/consultar-central`, marque a tarefa `[!]` e espere a central.
   5. Rode as verificações da skill `/verificar`.
   6. Se a tarefa mudou o que aparece na tela, gere a imagem da cena de prova, abra o arquivo e compare com `docs/VISUAL.md`. Anote em uma linha, na seção "Prova visual" do `tasks.md`, o que viu; a descrição completa vai para o report. Uma imagem errada impede o commit, como um teste que falha. Se a cena é nova, o teste da prova visual só passa com a imagem aprovada em `tests/visual/approved/`: mostre a imagem ao dono e, se ele aprovar, entregue o comando pronto para ele mesmo copiar o arquivo. A IA nunca copia nem altera essas imagens. A imagem precisa estar lá antes do `/fechar`, para a CI do PR ficar verde.
   7. Se tudo passou: marque `[x]`, escreva o resultado em uma frase e faça o commit `tipo(NNNN): Txx descrição`, com o detalhe da tarefa no corpo da mensagem, e o `git push`. Se a CI de uma tarefa falhar, corrija antes de fechar a tarefa seguinte.
   8. Se não passar depois de tentativas razoáveis: marque `[!]`, descreva o bloqueio em "Bloqueios" no `tasks.md`, pare e pergunte ao dono.
3. **Ao terminar todas as tarefas,** diga ao dono que a próxima etapa é `/revisar`.

## Durante o trabalho

- **Fora do escopo:** registre com `/propor` e não implemente. A exceção é algo que impede a tarefa atual: nesse caso, pare e pergunte.
- **Simplificação provisória nova:** registre em `docs/TECH_DEBT.md` com a fatia ou o marco de destino.
- **Biblioteca nova, mudança nos formatos da engine, no contrato de rede ou na arquitetura:** só com a aprovação do dono.
- **Dúvida sobre como o original funciona:** use `/consultar-central`. Nunca procure o código do MuMain.
- **Código ou XML do OpenMU reaproveitado:** mantenha o aviso da licença MIT junto do arquivo e registre a fonte no `plan.md`.
- **Sessão longa:** antes que o contexto fique grande demais, atualize "Notas para a próxima sessão" no `tasks.md`, faça o commit e sugira continuar numa sessão nova.

## Nunca

- Enfraquecer ou apagar um teste para "fazer passar".
- Desligar uma checagem, mudar uma trava sem aprovação ou usar `--no-verify`.
- Escrever um comportamento do original sem fonte na central.
- Dizer que algo passou sem ter rodado.
