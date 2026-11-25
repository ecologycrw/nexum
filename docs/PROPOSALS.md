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
- Revista na retrospectiva da fatia 0 (2026-11-25): continua aberta até o ponto de decisão.

### P-0008: a prova visual do M3 com as placas de vídeo por software da CI
- Origem: feedback do dono, depois da revisão da fatia 0 (a pergunta sobre placa de vídeo nas máquinas do GitHub)
- Tipo: decisão
- Evidência: o dono perguntou "o Github tem GPU para o meu plano free ? Tanto windows quanto linux ? E o que essa resposta impacta em nosso projeto ?" e, depois da resposta, pediu "Sim, registrar". As máquinas gratuitas do GitHub não têm placa de vídeo: a CI desenha com placas feitas por software, o lavapipe no Linux, o WARP no Windows e a placa virtual do macOS ([TD-0018](TECH_DEBT.md)). Na fatia 0, a imagem aprovada foi tirada no computador do dono (NVIDIA, Vulkan), e a captura da CI bateu com ela, com no máximo 1 nível de diferença, porque o cartão de teste só copia pixels. A partir do M3, com luz, névoa, texturas suavizadas e shaders, uma placa por software e uma placa de verdade podem desenhar pixels um pouco diferentes, e a mesma imagem aprovada pode não servir para as duas. Nem o [VISUAL](VISUAL.md) nem o TD-0016 (a comparação perceptual, M3) dizem onde a imagem aprovada é gerada, nem qual tolerância vale entre uma placa de verdade e uma por software. A decisão cabe no que o M3 manda investigar antes. Opções a comparar:
  - a imagem aprovada gerada na CI e conferida pelo dono;
  - uma imagem aprovada por tipo de placa;
  - a comparação perceptual com uma tolerância por cena, medida nas duas placas.

  As opções de ter uma placa de verdade na CI estão na P-0009.
- Prioridade sugerida: média (decidir antes do M3)
- Status: aberta
- Revista na retrospectiva da fatia 0 (2026-11-25): continua aberta até o planejamento do M3.

### P-0009: as opções para a CI ter uma placa de vídeo de verdade
- Origem: feedback do dono, na mesma conversa da P-0008
- Tipo: decisão
- Evidência: o dono pediu "Deixe registrado em algum lugar que você ache melhor, algo sobre a seção ali 'Se um dia a CI precisar de uma placa de verdade' também." Hoje a CI só confere se o desenho está certo. A fluidez e os orçamentos são medidos no computador do dono, e nenhuma placa mais fraca é testada (TD-0018, M7). Em 2026-11-21, as opções eram três:
  - **O computador do dono como máquina da CI** (o *runner* próprio do GitHub): sem custo, com a GTX 1060 de verdade. O repositório é público, então um PR de outra pessoa poderia rodar código no computador do dono. A máquina precisaria aceitar só os PRs do próprio projeto, e isso pede um ADR e mudanças nas travas (`.github/`).
  - **As máquinas do GitHub com placa de vídeo** (NVIDIA T4, no Windows e no Linux): existem só para organizações nos planos pagos Team ou Enterprise Cloud, e são cobradas por minuto mesmo em repositório público. Em 2024, eram cerca de US$ 0,05 por minuto no Linux e US$ 0,10 no Windows, e o GitHub anunciou mudanças de preço em 2026 ([GPU hosted runners](https://github.blog/changelog/2024-07-08-github-actions-gpu-hosted-runners-are-now-generally-available/), [preços dos runners](https://docs.github.com/en/billing/reference/actions-runner-pricing)).
  - **Continuar como hoje:** a correção na CI, e a fluidez e o desempenho no computador do dono.

  A recomendação dada ao dono foi continuar como hoje até o M3, e decidir junto com a P-0008 e com a medição do piso de hardware (TD-0018, M7).
- Prioridade sugerida: baixa (rever no M3 e no M7)
- Status: aberta
- Revista na retrospectiva da fatia 0 (2026-11-25): continua aberta, para rever junto com a P-0008.

## Processadas

### P-0012: enxugar o ambiente da IA (menos tokens e menos texto repetido)
- Origem: feedback do dono, em 2026-11-25, depois de duas análises do ambiente: uma sobre o consumo de tokens e outra sobre excesso nos agentes, hooks, skills e docs
- Tipo: regra, trava, skill, agente, doc e decisão (vários itens, para processar juntos no `/consolidar`)
- Evidência: o dono perguntou primeiro "o ambiente como está, acaba trazendo o uso de mais tokens que o normal ou o não?" e depois "os agents, hooks, skills, docs, specs e demais .md e etc são bem feitos para o que o projeto pede ou tem excesso?". Depois das respostas, pediu: "pode colocar todos os itens encontrados [...] juntando tudo em uma proposta só. Que eu vou usar o consolidar em seguida". As duas análises concluíram que a base é boa: os hooks, as checagens, a CI e os testes delas, as skills, os agentes e os docs vivos estão na medida e **não devem ser enfraquecidos**. O gasto e o excesso vêm dos itens abaixo.

  **Consumo de tokens**

  1. **A saída do `verify` é longa demais.** Medido em 2026-11-25: sem nada para compilar, o `py scripts/dev.py verify` escreveu cerca de 17 mil caracteres (uns 5 mil tokens). Uns dois terços disso é repetição: o aviso do vcpkg aparece 4 vezes, a configuração do CMake roda 4 vezes, e cerca de 150 linhas são de testes que passaram. Com código para compilar, entra uma linha por arquivo. O `verify` roda depois de cada tarefa e de novo no revisor (a fatia 0 teve 17 tarefas e várias correções), e tudo fica no contexto da sessão até o fim. **Ação:** um modo resumido no `scripts/dev.py`, com uma linha por passo e o detalhe só quando algo falha; o vcpkg sem o aviso de uso (`--no-print-usage`); e a configuração uma vez por tipo de build. O `verify` completo continua rodando depois de cada tarefa. Travas: `scripts/dev.py` e, se for preciso, `CMakePresets.json`; os testes de `scripts/checks/tests/test_dev.py`. **Condição para não perder qualidade:** o resumo mostra os números de cada passo (testes que passaram de quantos, em cada build, e os arquivos analisados); o log completo fica salvo num arquivo dentro de `build/`, para a IA abrir quando precisar; uma falha continua aparecendo inteira; e a CI não muda.
  2. **O `tasks.md` virou diário.** O da fatia 0 tem 78 KB (uns 20 mil tokens) e é lido inteiro a cada retomada, porque é a memória entre sessões. Há tarefas com parágrafos de mil caracteres, e o mesmo conteúdo se repete no report e nos commits. **Ação:** uma regra na skill `/implementar` e no `templates/tasks.md`: cada tarefa com 1 a 3 linhas (o estado, o resultado e o commit). O detalhe vai para a mensagem do commit, e as provas, para o report. Trava: `.claude/skills/`. **Condição para não perder qualidade:** encurta-se só o relato técnico. As decisões do dono (como a escolha dos segundos em `double` na T04 da fatia 0), os bloqueios, as correções pedidas e as "Notas para a próxima sessão" continuam no `tasks.md`, porque ele é a memória entre sessões.
  3. **A spec de referência é lida em pedaços grandes demais.** Ela tem 185 KB e 3.052 linhas (uns 50 mil tokens inteira). O `/planejar` pede só algumas partes, mas a ferramenta de leitura lê até 2.000 linhas por padrão. **Ação:** as skills (`/planejar`, `/consultar-central`) e o `auditor` passam a mandar achar o título da seção com a busca e ler só aquele trecho. A cópia em `docs/referencia/` não muda. Travas: `.claude/skills/` e `.claude/agents/`. **Condição para não perder qualidade:** a skill mantém a lista das seções que sempre se leem (a do marco na Parte VI, os subsistemas da Parte IV, as lacunas da Parte VIII e os números do Apêndice C que a fatia usar) e manda seguir as referências cruzadas que o trecho lido fizer.
  4. **O `PROPOSALS.md` é lido inteiro, mas quase tudo nele já foi processado.** São 181 linhas, das quais só cerca de 35 são propostas abertas, e o `/planejar` e o `/situacao` leem o arquivo todo. **Ação:** levar as processadas para um arquivo próprio (por exemplo `docs/PROPOSALS-processadas.md`), com o link no topo do `PROPOSALS.md`. Antes, conferir se a checagem `docs`, o hook do início da sessão e os testes deles dependem das processadas estarem no mesmo arquivo. **Condição para não perder qualidade:** a `/retrospectiva` e a escada de erros continuam lendo o arquivo das processadas quando precisarem do histórico, e a numeração `P-NNNN` continua contando as duas listas.
  5. **Retirado pelo dono (2026-11-25):** a ideia era o `auditor` usar um modelo mais barato. Ele roda só a cada cerca de 5 fatias, então a economia seria pequena, e um modelo mais fraco deixaria passar contradições sem que ninguém notasse. O `auditor` e o `revisor` continuam com o modelo principal. A numeração dos outros itens foi mantida.
  6. **Conectores do app que o Nexum não usa.** Toda sessão carrega a lista de skills e conectores gerais do app (documentos, Chrome, gráficos e outros), alguns milhares de tokens. **Ação do dono, fora do repositório:** desligar no app os conectores que o Nexum não usa.

  **Excesso de texto**

  7. **Os registros de cada fatia são grandes.** A fatia 0 gerou 130 KB de spec, plano, tarefas e report. O modelo do report tem 15 seções, e várias costumam sair "não se aplica". **Ação:** além do item 2, rever o `templates/report.md`, juntando as seções que andam juntas (por exemplo "Itens provisórios criados" com "Itens provisórios resolvidos", e "Travas alteradas" com "Testes removidos"). Trava: a checagem `docs` em `scripts/checks/`, que exige as seções. **Condição para não perder qualidade:** juntar muda só a arrumação. A checagem continua exigindo o conteúdo de cada parte, principalmente os testes removidos e as travas alteradas, que impedem a IA de enfraquecer um teste ou uma trava sem o dono ver.
  8. **A mesma regra está escrita em vários lugares.** A checklist de sala limpa está no `docs/WORKFLOW.md`, no Apêndice E da spec, na lista do `revisor` e no `templates/report.md`. A definição de pronto está no `AGENTS.md`, no `WORKFLOW.md`, na Parte V.6 da spec e na skill `/verificar`. O que exige aprovação está no `AGENTS.md` e no `WORKFLOW.md`. As cópias divergem com o tempo: o PR da consolidação de 2026-11-25 existe para corrigir contradições entre documentos, skills e checagens. **Ação:** cada regra com um único dono, e os outros lugares só apontam para ele. Por exemplo, a checklist de sala limpa fica só no `WORKFLOW.md`, e o que é proibido ou exige aprovação fica só no `AGENTS.md`, que é carregado em toda sessão. Travas: `AGENTS.md`, `.claude/skills/` e `.claude/agents/`. **Condição para não perder qualidade:** os resumos ficam onde são lidos sempre: no `AGENTS.md` e na lista do `revisor`, que trabalha sem o contexto da sessão e precisa ter a sala limpa na própria lista. Saem só as cópias dos documentos explicativos, como o `WORKFLOW.md`, que passam a apontar para o dono da regra.
  9. **Os ADRs 0001 a 0020 copiam as decisões D-01 a D-20 da spec de referência.** São uns 40 KB repetidos, e quando a spec mudou foi preciso criar 3 ADRs (0029 a 0031) só para acompanhar. **Ação (decisão do dono):** daqui para frente, ADR só para decisões do próprio Nexum e para desvios da spec; uma decisão que só segue a spec é citada pelo número D-xx. Os ADRs existentes ficam como estão, porque são documentos históricos. Muda o `docs/adr/README.md` e, se o dono quiser registrar a mudança de prática, um ADR novo. **Condição para não perder qualidade:** todo desvio da spec continua ganhando um ADR com o motivo.
  10. **A guarda do terminal bloqueia comandos que não escrevem em travas.** O `guard-bash.ps1` bloqueia qualquer comando que cite uma trava e tenha `>`, `Out-File` ou parecido, mesmo quando a escrita vai para outro lugar. Em 2026-11-25, guardar a saída do `verify` num arquivo temporário foi bloqueado, e a memória da IA já registrava o mesmo problema com um `>=` dentro de um comando de leitura. Cada bloqueio falso custa uma volta a mais. **Ação:** deixar de bloquear os casos seguros conhecidos. Travas: `.claude/hooks/` e `tests/hooks/`. **Condição para não perder qualidade (o item mais delicado, porque afrouxa uma trava de segurança):** saber com exatidão para onde um comando escreve é difícil (um nome de arquivo guardado numa variável passaria por uma regra fina), e a regra larga de hoje pega esses casos. Por isso, a regra não tenta adivinhar o destino. Ela libera só uma lista curta de casos seguros: a escrita na pasta temporária da sessão e o `>` que aparece dentro de texto entre aspas, como o `>=` de um filtro. Todos os 98 testes atuais continuam, e entram testes novos para os casos liberados e para as tentativas de escapar (o destino numa variável, a escrita encadeada depois de um caso liberado). Se o dono não aprovar, o item é descartado e a regra fica como está.
  11. **Observação de tempo, sem efeito nos tokens.** Cada ação da IA abre um PowerShell novo para as travas, e um comando de terminal abre dois. Medir quanto isso atrasa antes de propor qualquer mudança.

  **O que não muda:** os arquivos da fatia 0 (`specs/0000-fundacao/`), por pedido do dono ("como a spec da fatia 0 já está feita, então não acho que seria legal mexer nos conteúdos internos dela"); os ADRs existentes; e a cópia da spec em `docs/referencia/`. As regras novas valem a partir da próxima fatia.
  **Regra geral:** cada item vem com uma condição para não perder qualidade, e o `/consolidar` só aplica o item se puder cumprir essa condição.
- Prioridade sugerida: alta para os itens 1, 2 e 8; média para 3, 4, 7 e 10; baixa para 6, 9 e 11 (o item 5 foi retirado)
- Status: aceita
- Destino: processada no `/consolidar` de 2026-11-25 (branch `consolidacao/2026-11-25-ambiente`), com o "Aprovo tudo" do dono para a limpeza proposta depois da auditoria. A auditoria corrigiu três pontos da evidência: o modelo do report tinha 14 seções, não 15; a checagem `docs` nunca exigiu as seções de travas e de testes removidos, então o item 7 passou a acrescentar essa exigência em vez de "manter"; e o resumo do item 1 não pode valer na CI, que não guarda o log. Cada item:
  - **1:** feito (`ff3b4ee`). Fora da CI, o `scripts/dev.py` mostra só os números de cada passo e grava a saída completa em `build/dev-last.log`; uma falha aparece inteira, e `--full` ou a CI mostram tudo. Cada tipo de build é configurado uma vez por comando, e o vcpkg não imprime mais o aviso de uso. Medido no `quick`: cerca de 700 caracteres, contra cerca de 17 mil no `verify` de antes.
  - **2:** feito (`d9231a9`). O `/implementar` e o `templates/tasks.md` pedem no máximo 2 linhas depois da primeira linha de cada tarefa, que o hook lê, e o modelo ganhou a seção "Decisões do dono".
  - **3:** feito (`2b35510`). O `/planejar`, o `auditor` e o `/consultar-central` leem as specs longas por seção. O `/planejar` passou a ler também a Parte VII, como o README da referência já dizia, e os apêndices B e C quando a fatia os usar.
  - **4:** feito do jeito mais barato (`2b35510`). As processadas ficam no mesmo arquivo; o `/planejar` lê só até "Processadas", e o `/situacao` usa a contagem do hook.
  - **5:** retirado pelo dono antes da consolidação.
  - **6:** fica com o dono, fora do repositório: desligar no app os conectores que o Nexum não usa.
  - **7:** feito (`ba65502` e `9a5cd3b`). O modelo do report juntou as travas com os testes removidos e os itens provisórios criados com os resolvidos. A checagem `docs` passou a exigir a seção de travas e testes removidos; os reports antigos, como o da fatia 0, podem manter as duas seções separadas. O resumo do PR passou a contar as linhas removidas dos testes em PowerShell.
  - **8:** feito (`35d38ea`). O `AGENTS.md` ficou dono do que exige aprovação e do que é proibido; o `WORKFLOW.md`, da checklist de sala limpa e da definição de pronto. A seção do `AGENTS.md` virou "A prova de que passou", e o `WORKFLOW.md` corrigiu as camadas de travas para "1 a 4".
  - **9:** feito (`ba65502`). O `docs/adr/README.md` diz quando criar um ADR, e o `/fechar` aponta para ele. Os ADRs existentes não mudaram.
  - **10:** feito (`c63959b`). A guarda libera só a saída gravada na pasta temporária do Windows e o `>` entre aspas, quando o comando não roda outro comando por dentro. Os 98 casos continuam, com 30 novos, 22 deles tentativas de escapar. Continua bloqueada uma mensagem de commit que cite um comando de escrita (como `cp` ou `Out-File`) junto com o nome de uma trava; isso não estava no item e fica como está.
  - **11:** fica para medir quando a demora dos hooks incomodar; nada mudou.
  - **Também nesta consolidação:** a linha da INV-05 no `ARCHITECTURE.md` (`5730a9d`), e um passo novo no `/consolidar` e no `/propor` para as propostas tratadas numa consolidação.

### P-0011: trocar a cópia da spec de referência pela versão de 2026-11-22 da central
- Origem: o hook do início da sessão, em 2026-11-25 ("a spec de referencia mudou na central")
- Tipo: doc (a cópia em `docs/referencia/` é uma trava)
- Evidência: ao responder às perguntas do M2, a central corrigiu 3 trechos da spec em 2026-11-22: o `.att` tem 1 ou 2 bytes por célula (I.3.2 e I.3.3); a regra de bloqueio do A\* não é a que a doc do editor descreve (I.3.2); e a IV.15.1 passou a apontar para a spec de protocolo nova, com uma nota sobre os 13 passos. O front matter da central continua dizendo `atualizado: 2026-11-20`.
- Prioridade sugerida: alta (as correções valem para o M2)
- Status: aceita
- Destino: aplicada na retrospectiva da fatia 0, com o "sim" do dono ("Sim, aplicar"). A cópia ficou idêntica à da central, o que o hook do início da sessão confirmou, e o [README da referência](referencia/README.md) ganhou o hash novo e a linha da atualização. Nenhuma decisão mudou, e nenhum ADR novo foi preciso.

### P-0010: a janela do `MuClient` sem a tela preta no início
- Origem: feedback do dono, no teste da fatia 0
- Tipo: fatia (o começo do programa, no M1)
- Evidência: no teste, o dono viu a janela preta antes do cartão e escreveu: "O timing entre o [info] platform: video driver windows e o [info] render: GPU backend vulkan on NVIDIA GeForce GTX 1060 6GB (driver NVIDIA 582.78.0.0) gera esse delay inicial que faz com que a primeira tela fique preta". Depois, perguntou "quais são as soluções pra isso, ou o que engines ou jogos fazem" e escolheu "Virar proposta".
  - **A causa:** o `MuClient` abre a janela antes de ligar a placa de vídeo (`Games/MuOnline/App/Private/FoundationApp.cpp`). Até o primeiro quadro, a janela não tem nada para mostrar, e o Windows a pinta de preto. Não quebra nenhum critério da fatia 0: acontece uma vez, no início, e o ritmo do laço segue normal depois.
  - **A medição** em 2026-11-21, no computador do dono, com `--frames 1` e a hora de cada linha do log, em 5 execuções:

    | Passo | Vulkan | Direct3D 12 |
    |---|---|---|
    | o vídeo ligado e a janela aberta | 0,07 s | 0,06 s |
    | a placa de vídeo ligada (a linha `GPU backend ...`) | 0,44 s | 0,69 s |
    | o primeiro quadro desenhado (a imagem da janela preparada, o cartão enviado à placa e o primeiro desenho) | 0,75 s | 0,72 s |

    A janela fica preta por uns 0,7 s. A maior parte do tempo é o driver da placa começando, e o Nexum não controla isso.
  - **As soluções** levadas ao dono:
    1. a janela criada escondida e mostrada só depois do primeiro quadro;
    2. a placa de vídeo ligada antes de abrir a janela (o preto cairia para uns 0,3 s);
    3. uma tela de abertura com um logo, desenhada pelo sistema, sem a placa;
    4. a janela pintada com a cor de fundo do jogo no lugar do preto;
    5. uma tela de carregamento, com o resto carregando em paralelo pelos *workers*.
  - **O que as engines e os jogos fazem:** Unity, Unreal e Godot mostram uma tela de abertura configurável enquanto a engine começa (a solução 3). Os programas de desktop feitos com o Electron criam a janela escondida e a mostram quando ela está pronta (a solução 1). Os jogos grandes cobrem o começo com logos e vídeos e depois usam telas de carregamento (as soluções 3 e 5).
  - **A recomendação dada ao dono:**
    - as soluções 1 e 2 juntas, no M1. São simples, tiram o preto e não mudam a arquitetura. Pedem uma opção nova na `Window` (`Engine/Platform`) para abrir escondida e mostrar depois, e a ordem nova no início do `MuClient`;
    - a solução 5 também no M1, quando o `MuClient` começar a carregar arquivos de verdade. O quadro nunca espera por disco (INV-06), então a tela de carregamento vai ser necessária de qualquer jeito;
    - a solução 3 só se o início passar de 1 a 2 segundos antes de qualquer coisa aparecer.
- Prioridade sugerida: baixa (é só aparência e dura menos de 1 segundo; fazer no M1, junto com o início do visualizador)
- Status: aceita
- Destino: o M1, junto com o início do visualizador: as soluções 1 e 2, e a 5 com a tela de carregamento. Decidido na retrospectiva da fatia 0 (2026-11-25).

### P-0005: a checagem `structure` conferir também as bibliotecas opcionais do vcpkg
- Origem: fatia 0, T16 (o registro do Tracy no THIRD_PARTY)
- Tipo: trava (a checagem fica em `scripts/checks/`)
- Evidência: a checagem compara o THIRD_PARTY só com as bibliotecas principais do `vcpkg.json`. O Tracy fica numa opção (*feature*) do `vcpkg.json` e, por isso, não é conferido: ele foi registrado numa lista à parte, fora da tabela que a checagem lê. Uma biblioteca opcional nova poderia entrar sem registro e sem licença conferida.
- Prioridade sugerida: baixa
- Status: aceita
- Destino: trava, antes da próxima biblioteca opcional do `vcpkg.json`, provavelmente o ImGui do modo de autoria, no M1. Decidido na retrospectiva da fatia 0 (2026-11-25).

### P-0004: o contador de alocações ver também a memória que o SDL pede
- Origem: fatia 0, T15 (os 35 pedidos de memória do macOS)
- Tipo: trava (o contador fica em `tests/architecture/frame_allocations/`)
- Evidência: o teste do quadro (CA-09) só conta os pedidos feitos por `new`. O SDL pede memória por `malloc`, que o contador não vê. Por isso, a fila sem limite do desenho sem janela só apareceu no macOS, onde o driver da Apple pede memória por `new`. No Windows e no Linux, a mesma fila pode ter crescido sem que o teste visse. O SDL permite trocar as suas funções de memória (`SDL_SetMemoryFunctions`) antes do primeiro pedido. Com isso, o contador veria os pedidos do SDL nos 3 sistemas. A troca seria feita só nos programas de teste e precisa respeitar a INV-10 (cada biblioteca externa atrás de um único módulo).
- Prioridade sugerida: média
- Status: aceita
- Destino: trava, na primeira fatia do M1, junto com o [TD-0019](TECH_DEBT.md), quando os shaders e os modelos trouxerem mais trabalho do SDL para dentro do quadro. Decidido na retrospectiva da fatia 0 (2026-11-25).

### P-0007: o `MuClient` nos testes sem as janelas de falha do Windows, e um tempo limite geral para os testes
- Origem: feedback do dono na fatia 0, depois da P-0006, comparando com uma proposta de outro projeto dele
- Tipo: trava (os presets de teste ficam no `CMakePresets.json`)
- Evidência: o dono mandou a proposta de outro projeto, em que um programa de teste abriu a janela "Debug Assertion Failed" do Windows e ficou parado até alguém clicar, e pediu: "Verifique pra mim, se o que a proposta propõe já existe funcionando ou documentado no Nexum Project". No Nexum, o `main` comum dos programas de teste (`tests/support/DoctestMain.cpp`, desde o commit `739471f`) já escreve essas falhas como texto (`_set_abort_behavior` e `_CrtSetReportMode`), com um teste em `nexum_test_main_tests`, e o doctest já transforma em texto as quedas graves no Windows. Ficam de fora:
  1. o `MuClient` não usa esse `main`. Os testes do CTest que rodam o `MuClient` do build de desenvolvimento (a versão, os argumentos, os 300 quadros e a captura) abririam a janela se um assert do C++ ou o `NEXUM_ASSERT`, que chama `std::abort`, disparasse dentro do jogo, e o teste ficaria parado;
  2. não há tempo limite geral para os testes: nem os presets, nem o `scripts/dev.py`, nem a CI definem um. Os testes `muclient_reports_its_version`, `muclient_reports_the_build_kind` e `muclient_refuses_unknown_arguments` rodam o `MuClient` direto, sem limite; os outros passam por scripts com 120 ou 300 segundos. Na CI, um teste parado só seria cortado pelo limite do GitHub, de 6 horas;
  3. a regra não está escrita em nenhum documento, só no comentário do código e no teste.

  Sugestão: um tempo limite geral nos presets de teste, como rede de segurança para qualquer travamento; o `MuClient` de desenvolvimento escrever as falhas como texto quando roda dentro de um teste, mas continuar abrindo a janela quando um desenvolvedor o roda à mão, porque ela ajuda a investigar; e uma linha no `docs/setup/DEV.md`
- Prioridade sugerida: média
- Status: aceita
- Destino: trava e código da engine, aplicados na própria branch `fatia/0000-fundacao`, antes do `/revisar`, a pedido do dono ("Mas quero que você já analise a proposta P-0007 agora."). Mudaram:
  - o `CMakePresets.json`: os presets de teste dão a cada teste no máximo 300 segundos e ligam a variável `NEXUM_FAILURES_AS_TEXT`;
  - o `Engine/Platform` ganhou o `Nexum/Platform/FailureReports.hpp`, que, com a variável ligada, desliga as janelas do `abort` e dos asserts da biblioteca do C, encerra na hora quando uma função da biblioteca do C recebe um argumento inválido e desliga a janela de queda do Windows. Um teste novo (`FailureReportsTest.cpp`) confere a variável e o modo das falhas;
  - o `MuClient` e o `visualproof` chamam essa função na primeira linha do `main`. Ela não ficou no `RunApplication`, porque a versão, os argumentos e a captura do `MuClient` terminam antes dele;
  - o `docs/setup/DEV.md` e o `docs/ARCHITECTURE.md` explicam a regra.

  Provas com uma falha de propósito no `MuClient`, desfeita depois. Um índice fora de uma lista saiu como texto ("Assertion failed: vector subscript out of range") em 0,5 s, sem janela; sem a janela de queda desligada, o mesmo teste levou 4 a 5 s, e 28 s na primeira vez. Uma string nula passada ao `fputs` saiu como "a function of the C runtime received an invalid parameter" em 1 s. Nos programas de teste comuns, a mesma falha já terminava em 0,2 s, porque o doctest desliga a janela de queda sozinho. O tempo limite aparece no CTest como "Test timeout computed to be: 300"

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
- Status: aceita
- Destino: trava e skill, aplicadas na própria branch `fatia/0000-fundacao`, antes do `/revisar`, a pedido do dono ("Faça melhor, já analise essa proposta agora e aqui nessa sessão. Assim na nova sessão as coisas vão funcionar corretamente desde já."). Mudaram:
  - o hook `session-start`, que escreve a etapa, o próximo passo e quem age, com os avisos dos itens 2 e 3;
  - os testes dos hooks (`tests/hooks/session-start.tests.ps1`), com um caso por etapa e por aviso;
  - o modelo `templates/tasks.md` e o `tasks.md` da fatia 0, que ganharam a lista "Etapas";
  - as skills `/revisar`, `/fechar` e `/retrospectiva`, que marcam a sua etapa, e a `/situacao`, que usa a linha do hook;
  - o `docs/WORKFLOW.md`, que diz onde fica a etapa.

  O item 5 ficou de fora: o campo `fase` do STATUS só diz se há uma fatia em andamento, muda só no `/planejar` e na `/retrospectiva`, e a checagem `docs` exige o campo. Ele não estava errado; a etapa fina é que não tinha onde ficar, e agora fica no `tasks.md`.

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
