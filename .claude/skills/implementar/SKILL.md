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
2. **Uma tarefa por vez,** sempre a próxima pendente:
   1. Marque `[~]` no `tasks.md`.
   2. Escreva o teste primeiro, quando possível. O teste que prova um critério leva a etiqueta no começo do nome: `TEST_CASE("[NNNN-CA-xx] ...")`. Os exemplos esperados da seção "Suficiência das descrições" do `plan.md` viram testes.
   3. Implemente seguindo `docs/CONVENTIONS.md` e o código de referência citado lá. Respeite as invariantes de `docs/ARCHITECTURE.md`.
   4. **Sala limpa.** Se a tarefa reproduz um comportamento do original, escreva a partir da spec ou do achado listado em "Fontes da central usadas" do `plan.md`, nunca de memória e nunca do código do MuMain. Toda constante (tamanho, offset, tempo, código de pacote) vem de uma spec que cita a origem. Confira a checklist de `docs/WORKFLOW.md` (seção "Sala limpa"). Se a descrição não bastar, pare: use `/consultar-central`, marque a tarefa `[!]` e espere a central.
   5. Rode as verificações da skill `/verificar`.
   6. Se a tarefa mudou o que aparece na tela, gere a imagem da cena de prova, abra o arquivo e compare com `docs/VISUAL.md`. Anote no `tasks.md` o que viu. Uma imagem errada impede o commit, como um teste que falha.
   7. Se tudo passou: marque `[x]` e faça o commit `tipo(NNNN): Txx descrição`. Se houver remoto, `git push`.
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
