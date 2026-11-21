# Propostas

Fila única de tudo o que surgiu fora da tarefa atual: ideias, problemas, regras que estão faltando e lições aprendidas. A fila é processada na `/retrospectiva`, e cada entrada vira fatia, decisão, regra ou trava, ou é descartada. Ver [WORKFLOW.md](WORKFLOW.md#propostas-como-as-necessidades-aparecem).

Formato de cada entrada (o modelo completo fica em `templates/proposal.md`):

```markdown
### P-0001: título curto
- Origem: fatia e tarefa, feedback do dono, medição ou checagem
- Tipo: fatia | regra | trava | skill | agente | doc | decisão | bug
- Evidência: o que mostra que isso é necessário
- Prioridade sugerida: alta | média | baixa
- Status: aberta
```

## Abertas

### P-0001: separar os jogos em repositórios próprios no ponto de decisão do M7
- Origem: feedback do dono, depois do merge da preparação
- Tipo: decisão
- Evidência: o dono perguntou "Games deveria ficar onde está por enquanto e no futuro fazer um Documents/Nexum Projects ?" e, depois da explicação, pediu "Sim, registre". Hoje a spec põe as três camadas num repositório só ([ADR 0002](adr/0002-tres-camadas.md)), o que permite mudar a engine e o módulo MU na mesma fatia enquanto a API da engine ainda muda muito. A ideia é revisar isso no ponto de decisão depois do M7 (pergunta 4 do [ROADMAP](ROADMAP.md#-ponto-de-decisão)): o Nexum (Engine, Kit e Tools) viraria uma biblioteca com versão, e cada jogo teria o próprio repositório, por exemplo numa pasta `Nexum Projects`. A separação já é barata, porque a Engine e o Kit nunca incluem nada de `Games/` e o CMake só expõe as pastas `Public/`. Exigiria um ADR com as regras de versão, a forma de o jogo instalar a engine e a CI de cada lado
- Prioridade sugerida: baixa (decidir no ponto de decisão do M7)
- Status: aberta

### P-0004: o contador de alocações ver também a memória que o SDL pede
- Origem: fatia 0, T15 (os 35 pedidos de memória do macOS)
- Tipo: trava (o contador fica em `tests/architecture/frame_allocations/`)
- Evidência: o teste do quadro (CA-09) só conta os pedidos feitos por `new`. O SDL pede memória por `malloc`, que o contador não vê. Por isso, a fila sem limite do desenho sem janela só apareceu no macOS, onde o driver da Apple pede memória por `new`. No Windows e no Linux, a mesma fila pode ter crescido sem que o teste visse. O SDL permite trocar as suas funções de memória (`SDL_SetMemoryFunctions`) antes do primeiro pedido. Com isso, o contador veria os pedidos do SDL nos 3 sistemas. A troca seria feita só nos programas de teste e precisa respeitar a INV-10 (cada biblioteca externa atrás de um único módulo).
- Prioridade sugerida: média
- Status: aberta

### P-0005: a checagem `structure` conferir também as bibliotecas opcionais do vcpkg
- Origem: fatia 0, T16 (o registro do Tracy no THIRD_PARTY)
- Tipo: trava (a checagem fica em `scripts/checks/`)
- Evidência: a checagem compara o THIRD_PARTY só com as bibliotecas principais do `vcpkg.json`. O Tracy fica numa opção (*feature*) do `vcpkg.json` e, por isso, não é conferido: ele foi registrado numa lista à parte, fora da tabela que a checagem lê. Uma biblioteca opcional nova poderia entrar sem registro e sem licença conferida.
- Prioridade sugerida: baixa
- Status: aberta

### P-0006: a etapa da fatia calculada a partir do tasks.md, para o ciclo inteiro
- Origem: feedback do dono na fatia 0, depois da T17
- Tipo: trava (o hook do início da sessão, as skills do ciclo e os testes dos hooks)
- Evidência: nas palavras do dono, "quando todas as tarefas da fatia são concluídas, uma nova sessão quando chamado o /situacao só sabe que tem o /revisar pendente por causa das tarefas concluídas", "assim como para o restante do fluxo que aparece no WORKFLOW". Depois da última tarefa, nenhuma etapa deixa uma marca que o hook leia: a revisão, o fechamento, o teste do dono, a retrospectiva e o merge. O hook só diz "Todas as tarefas estao feitas". Uma sessão nova depois do `/revisar` não saberia que a revisão já foi feita. O campo `fase` do STATUS, que ninguém atualiza, ainda diz `em-andamento`, com a data de 2026-11-20. Proposta, seguindo a ideia da P-0003 (o estado calculado, não copiado):
  1. o `tasks.md` (e o modelo dele) ganha, depois das tarefas, uma lista de etapas: Revisão, Fechamento, Teste do dono e Retrospectiva. Cada skill marca a sua ao terminar, com a data e o commit; o teste do dono é marcado quando ele conta o resultado;
  2. o hook do início da sessão lê essa lista, a spec e as tarefas, e mostra a etapa, o próximo passo e quem age (o dono ou a IA), para o ciclo inteiro do WORKFLOW: sem fatia, spec em rascunho, tarefas, tarefa bloqueada, revisão, fechamento, teste do dono, retrospectiva, merge e a branch já na `main`;
  3. o hook avisa quando algo não bate: o fechamento marcado sem o `report.md`, uma mudança no código depois da revisão, ou a branch já na `main` com o STATUS ainda apontando para a fatia;
  4. o `/situacao` usa essa linha, em vez de deduzir a etapa;
  5. o campo `fase` sai do STATUS, pelo mesmo motivo da P-0003;
  6. um teste por etapa nos testes dos hooks, e uma linha no WORKFLOW dizendo onde a etapa fica.
  Até a mudança, a linha "Onde parou" das notas do `tasks.md` serve de apoio, atualizada por cada etapa.
- Prioridade sugerida: alta (processar na `/retrospectiva` da fatia 0, no mesmo PR, para a fatia 1 já começar com isso)
- Status: aberta

## Processadas

### P-0003: o estado da fatia calculado a partir do tasks.md, sem repetição
- Origem: feedback do dono na fatia 0, depois da T03
- Tipo: trava
- Evidência: depois das tarefas T01 a T03, o `tasks.md` estava certo, mas o STATUS ainda dizia que a próxima ação "começa pela sondagem", e a memória da IA também. Nenhum lugar dizia que o aviso automático da CI vale só para a sessão que o ligou. Tudo foi corrigido à mão no commit `0e51420`, e a própria correção repetiu no STATUS "3 de 17 tarefas feitas" e "a partir da T04", que vão envelhecer de novo. O dono perguntou "Acha que devemos ter uma skil ou alguma mudança nos docs para quando por exemplo, isso acontecer ? exemplo, uma tarefa é terminada aqui, mas não tenho certeza que existem arquivos desatualizados que fará com que a nova sessão não reconheça as coisas corretamente.", depois "Você acha que as suas 3 sugestões seria melhor que do jeito que está ?" e, com a versão menor, "Sim, registrar". A proposta:
  1. o hook do início da sessão lê o `tasks.md` da fatia atual e mostra quantas tarefas estão feitas, em andamento, bloqueadas e pendentes, e qual é a próxima, com um teste nos testes dos hooks;
  2. uma regra: o STATUS aponta para o `tasks.md` e não repete números de tarefa nem contagens. O "3 de 17" e o "a partir da T04" saem do STATUS;
  3. uma linha no `/situacao`: dizer se o PR está em rascunho e lembrar que o aviso automático da CI vale só para a sessão que o ligou.

  Fica de fora uma checagem da fase do STATUS contra o status da spec: ela não teria pegado este caso, porque o erro estava numa frase. Muda travas: o hook `session-start`, os testes dos hooks e a skill `/situacao`
- Prioridade sugerida: média (aplicar antes de muitas sessões novas na fatia 0)
- Status: aceita
- Destino: trava, regra e skill, aplicadas na branch `consolidacao/proposta-p-0003` a pedido do dono ("Bom, quero que você adiante assim como foi com a P-0002"). A branch sai da `fatia/0000-fundacao`, e o PR dela entra nessa branch, para as sessões da fatia 0 já terem o resumo novo. Mudaram:
  - o hook `session-start`, que conta as tarefas e mostra a próxima;
  - os testes dos hooks (`tests/hooks/session-start.tests.ps1`);
  - a skill `/situacao` (o andamento vem do `tasks.md`; o PR em rascunho e o aviso automático por sessão);
  - o `docs/WORKFLOW.md` (a regra) e o STATUS, que deixou de repetir números de tarefa.

### P-0002: conferir se a descrição da central basta antes de aprovar a spec
- Origem: feedback do dono, depois do merge da preparação
- Tipo: skill
- Evidência: o dono perguntou "como que o planejar confia que a descrição basta para o comportamento e não que a descrição está faltando ou resumida demais ?" e pediu "Sim, registre". Hoje a primeira avaliação é julgamento da IA no passo 3 do `/planejar`. As outras redes (parar no `/implementar`, o revisor, os testes de paridade) pegam a falta tarde, depois de parte do código escrito. A proposta acrescenta ao `/planejar` e ao `templates/plan.md`:
  1. uma checklist por comportamento reproduzido do original (entradas e saídas, ordem dos passos, tempos por tick ou por quadro, limites e casos de erro, números com fonte), em que cada item vazio vira uma pergunta para a central antes da aprovação;
  2. dois ou três exemplos de entrada e resultado esperado, escritos no plano só a partir da descrição; nos casos delicados, pedidos à central.

  A checagem `docs` pode passar a exigir essa seção nos planos aprovados. Muda uma trava (a skill) e o modelo do plano
- Prioridade sugerida: média (aplicar antes do M1 e do M2, os marcos com mais comportamento do original)
- Status: aceita
- Destino: trava e skill, aplicadas na branch `consolidacao/proposta-p-0002` a pedido do dono ("não conseguiria adiantar já a P-0002 aqui nesta sessão mesmo ?"). Mudaram:
  - a skill `/planejar` (passo 3);
  - o `/implementar` (os exemplos viram testes);
  - o revisor;
  - o `templates/plan.md` (seção "Suficiência das descrições");
  - a checagem `docs`, que exige a seção e recusa um plano aprovado com "Perguntas abertas para a central";
  - o `docs/WORKFLOW.md` e o `docs/setup/DEV.md`.
