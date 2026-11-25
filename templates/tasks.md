# Tarefas da fatia N

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui. Por isso ele é curto. O detalhe de cada tarefa vai para a mensagem do commit, e as provas, para o report.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

Cada tarefa tem a primeira linha no formato abaixo, que o hook do início da sessão lê, e no máximo 2 linhas depois dela: o resultado em uma frase e o commit.

- [ ] T01: O que fazer (CA-01). Verificação: como saber que terminou.
- [ ] T02: O que fazer (CA-02). Verificação: como saber que terminou.

## Etapas

Depois das tarefas, cada etapa do ciclo marca a sua linha quando termina, com a data e, na revisão, o commit. O hook do início da sessão lê esta lista para dizer a etapa e o próximo passo (`docs/WORKFLOW.md`).

- [ ] Revisão (`/revisar`)
- [ ] Fechamento (`/fechar`)
- [ ] Teste do dono
- [ ] Retrospectiva (`/retrospectiva`)

## Decisões do dono

Cada escolha que o dono fez durante a fatia, com as palavras dele e a tarefa: o que mudou e por quê. Nenhuma decisão do dono fica só no meio do texto de uma tarefa.

*(nenhuma)*

## Bloqueios

*(nenhum)*

## Prova visual

Uma linha por imagem da cena de prova: o que a IA viu, inclusive o que ficou ruim. A descrição completa vai para o report. Se a fatia não muda o que aparece na tela: "não se aplica".

## Revisão

Achados do revisor e o que foi feito com cada um, uma linha por achado. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

Contexto que a próxima sessão precisa saber e que não está em outro lugar.

Os desvios do plano e as travas alteradas não ficam aqui: vão direto para o report.
