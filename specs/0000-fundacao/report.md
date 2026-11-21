# Relatório da fatia 0: Fundação

## Resumo para o dono

O `MuClient` abre uma janela e desenha pela placa de vídeo o **cartão de teste**: 8 faixas de cores, uma escala de cinza, a marca em "L" no canto de cima à esquerda e um quadrado que anda de um lado para o outro e troca de cor uma vez por segundo. A lógica anda em ticks fixos de 40 ms (25 por segundo), e o desenho fica entre dois ticks, por isso o quadrado anda liso no ritmo do monitor.

A engine ganhou as peças de base que todo o resto vai usar: o relógio de ticks, os temporizadores, os handles, a memória do quadro, o assert, o log e o profiler. Ganhou também a ferramenta de prova visual, que compara a imagem nova com a que você aprovou. A CI passa nos 3 sistemas (Windows, Linux e macOS), já desenhando pela placa de vídeo, que nas máquinas do GitHub é feita por software.

## Critérios de aceitação

As provas automáticas rodam em todo `py scripts/dev.py verify` e na CI. A última execução completa da CI antes do fechamento foi a [44917479757](https://github.com/ecologycrw/nexum/actions/runs/44917479757), do commit `d6f732a`, verde nos 10 checks. O commit do fechamento só muda documentos, e a CI dele roda no [PR #5](https://github.com/ecologycrw/nexum/pull/5).

| CA | Prova | Resultado |
|---|---|---|
| CA-01 | teste `ten simulated minutes of irregular frames give exactly 15000 ticks` (etiqueta `[0000-CA-01]`, `Engine/Time/Tests/TickClockTest.cpp`): 15.000 ticks, a posição 45.000 e α 0 | passou |
| CA-02 | 6 testes com a etiqueta `[0000-CA-02]` em `Engine/Time/Tests/TickClockTest.cpp`, com os 4 exemplos do plano: até 5 ticks por quadro, o descarte contado, o α, a pausa (sem ticks, α parado e o tempo pausado descartado) e o avanço tick a tick | passou |
| CA-03 | testes com a etiqueta `[0000-CA-03]` em `TimersTest.cpp`, `TickDiagnosticsTest.cpp` e `MonotonicClockTest.cpp` (`Engine/Time/Tests`): o temporizador de parede dispara com a simulação pausada, o de tick conta só ticks, e o relatório tem a faixa acima de 8 ms | passou |
| CA-04 | testes com a etiqueta `[0000-CA-04]` em `HandlePoolTest.cpp`, `FrameArenaTest.cpp` e `AssertTest.cpp` (`Engine/Core/Tests`) e em `Engine/Render/Tests/TextureStoreTest.cpp`; o assert falso pelo teste do CTest `nexum_core_assert_failure_tests`, que exige a mensagem com a condição, o arquivo e a linha | passou |
| CA-05 | testes com a etiqueta `[0000-CA-05]` em `LoggerTest.cpp`, `TerminalLogSinkTest.cpp` e `ProfilerTest.cpp` (`Engine/Debug/Tests`) e em `Engine/Platform/Tests/SdlLogRouteTest.cpp`; os 3 testes do CTest `nexum_debug_profiler_*` (o build de jogador recusa o profiler ligado); o passo "Build with the profiler" da CI, no Linux (Clang e GCC) e no macOS | passou. No Windows, o passo do profiler não roda na CI ([TD-0012](../../docs/TECH_DEBT.md)) |
| CA-06 | testes com a etiqueta `[0000-CA-06]` em `GpuDriverOrderTest.cpp` e `GpuDeviceTest.cpp` (`Engine/Render/Tests`) e em `Games/MuOnline/Tests/ClientOptionsTest.cpp`; os testes do CTest `muclient_falls_back_to_direct3d12_without_vulkan` (Windows), `muclient_refuses_an_unknown_gpu_driver` e a linha do backend exigida pelo `muclient_draws_300_frames_and_closes` | passou. O log da CI não mostra qual backend rodou quando o teste passa (ver "Revisão", item 7) |
| CA-07 | teste do CTest `visual_proof_0000_test_card` (a captura do `MuClient --capture` e o `visualproof` com tolerância 1, comparando com `tests/visual/approved/0000-test-card.png`), e os testes com a etiqueta `[0000-CA-07]` em `TextureCopyTest.cpp`, `PngTest.cpp`, `FoundationScreenTest.cpp` e `ClientOptionsTest.cpp` | passou nos 3 sistemas da CI |
| CA-08 | testes com a etiqueta `[0000-CA-08]` em `Tools/VisualProof/Tests/ImageComparisonTest.cpp`: iguais, dentro da tolerância, tamanhos diferentes, um pixel acima da tolerância, arquivo ilegível e imagem de uma cor só, cada um com a sua mensagem | passou |
| CA-09 | teste com a etiqueta `[0000-CA-09]` em `Games/MuOnline/Tests/FrameAllocationTest.cpp` (programa `muonline_frame_allocation_tests`): zero pedidos de memória em 1.000 quadros, depois de 3 de aquecimento | passou nos 3 sistemas. A prova vale para o `new` e o `new[]` da linha do quadro; a memória que o SDL pede por `malloc` não é contada ([TD-0019](../../docs/TECH_DEBT.md)) |
| CA-10 | teste do CTest `muclient_draws_300_frames_and_closes` e o teste com a etiqueta `[0000-CA-10]` em `Engine/Render/Tests/WindowTargetTest.cpp`. No Linux, o teste exige `platform: video driver x11`, dentro do Xvfb | passou nos 3 sistemas da CI |
| CA-11 | verificação manual do dono, pelo passo a passo de "O que você deve testar". Antes, na T13, a IA abriu a janela aqui, com o Vulkan na GTX 1060: as fotos mostraram o cartão certo e o quadrado andando; mudar o tamanho, minimizar e restaurar não distorceu o cartão; fechar terminou o programa com o código 0; e o log disse `vulkan` | aguarda o teste do dono |
| CA-12 | verificação manual da IA nesta sessão, nos três pontos do critério de pronto do M0: (1) a CI verde nos 3 sistemas, na execução citada acima; (2) o CA-01 passou; (3) um `#include "Mu/Foundation/FoundationScene.hpp"` posto de propósito num arquivo novo em `Engine/Core/Private/` foi reprovado pela checagem `architecture` com o código 1 e a mensagem `this code must not depend on the MU module: the dependencies only go down (INV-02)`. O arquivo foi apagado logo depois, e a checagem voltou a passar. Os testes automáticos da checagem (`scripts/checks/tests/test_architecture_check.py`) também rodam em todo `verify` | confirmado |
| CA-13 | verificação manual dos documentos na T16, aprovada pelo dono, e a checagem `docs`: a [VISÃO](../../docs/VISION.md#piso-de-hardware) lista os requisitos do SDL GPU 3.4.20; o TD-0005 ficou só com o defeito do Direct3D 12, com destino M14; o TD-0003 e o TD-0006 saíram, e o que sobrou deles virou o TD-0010 e o TD-0016; o Tracy está no [THIRD_PARTY](../../docs/THIRD_PARTY.md) | confirmado |

## Paridade

Não se aplica: nenhum nível de P0 a P3 é medido nesta fatia. O ritmo de 25 ticks por segundo, que vem do original (I.2 da spec de referência), é conferido pelo teste de tempo (CA-01).

## Prova visual

- **Cena de prova:** o cartão de teste em 640×480, no tick 50, com α 0,5, desenhado sem janela. A captura não usa o relógio de verdade: ela entrega à tela 10 quadros de 0,2 s e um de 0,02 s, então a mesma cena dá sempre a mesma imagem.
- **Imagem aprovada:** `tests/visual/approved/0000-test-card.png`, aprovada e copiada para a pasta pelo dono em 2026-11-21.
- **O que a IA viu na captura** (Vulkan, GTX 1060):
  - as 8 faixas na ordem da spec (branco, amarelo, ciano, verde, magenta, vermelho, azul e preto), com cores puras e bordas retas;
  - a escala de cinza do preto, à esquerda, ao branco, à direita, sem tom de cor e sem degraus visíveis;
  - o "L" branco sobre um quadrado preto, no canto de cima à esquerda, em pé e sem espelhar;
  - o quadrado na terceira cor, em x = 198, sobre a cor de fundo azul-acinzentada escura.

  Nada ruim à vista.
- **Comparação:** no máximo 1 nível por canal e nenhum pixel fora disso. Aqui, três capturas seguidas, uma delas pelo Direct3D 12, deram exatamente a mesma imagem (diferença 0). Na CI, as placas por software dos 3 sistemas (o lavapipe no Linux e a placa virtual do macOS, por exemplo) passaram na comparação com a imagem tirada aqui. No Windows da CI, o log não diz qual backend rodou (ver "Revisão", item 7).
- **O revisor** abriu a captura e a imagem aprovada e não fez nenhum achado sobre elas.
- **Fluidez, só como referência** (as metas valem a partir do M3): no computador do dono, com o Vulkan na GTX 1060 e a janela visível, 300 quadros deram **16,7 ms de média, 17,4 ms no pior 1% e 17,5 ms no maior quadro**, que é o ritmo do monitor de 60 Hz. O laço rodou 25,0 ticks por segundo, sem ticks descartados.
- **Processador com a janela minimizada:** com o Vulkan, o laço ocupa 92% de um núcleo, contra 9% com a janela visível ([TD-0015](../../docs/TECH_DEBT.md), destino M1).

## Sala limpa

A fatia implementa um único comportamento que vem do original: o ritmo de 25 ticks por segundo do laço (T04 e T05). As outras tarefas são infraestrutura da engine e não reproduzem nada do MU. A checklist foi conferida para a T04 e a T05:

- [x] Escrito a partir de specs e achados da central, listados no `plan.md`, e não do código do MuMain. As fontes são a I.2, a D-05, a III.4 e a IV.2 da spec de referência, na seção "Fontes da central usadas" do [plano](plan.md).
- [x] Nenhum código do MuMain aberto durante a escrita. O hook `guard-central` bloqueia a pasta `Client/` da central.
- [x] Nenhum nome de variável, comentário ou estrutura copiado do MuMain.
- [x] Constantes numéricas com a fonte citada numa spec: os 40 ms, os 25 ticks por segundo e os 5 ticks por quadro vêm da D-05, e a faixa de 8 ms do histograma vem da D-06. Os números do cartão de teste (640×480, as cores, os 4 pixels por tick) são escolhas da spec da fatia, não do original.
- [x] Código ou XML do OpenMU com o aviso da licença MIT: não se aplica, porque nenhum código do OpenMU foi usado.
- [x] Nenhum asset da Webzen no commit. A única imagem é o cartão de teste, desenhado pelo próprio Nexum.
- [x] Lacunas levadas à central antes de implementar: não houve. O plano registra "Perguntas abertas para a central: nenhuma". O tratamento da pausa é uma decisão da engine, não do original.

Na T15, a IA leu no GitHub o código do SDL 3.4.20 (`src/gpu/metal/SDL_gpu_metal.m` e `src/gpu/SDL_gpu.c`) para entender o estoque de buffers do Metal. O SDL é uma biblioteca externa, com a licença zlib, e não é código do original. Nada dele foi copiado.

## O que você deve testar

No Windows, num terminal aberto na pasta do Nexum, na branch `fatia/0000-fundacao`:

1. Compile o build de desenvolvimento:

   ```powershell
   py scripts/dev.py build
   ```

2. Abra o jogo:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe
   ```

3. Confira na janela:
   - as 8 faixas de cores, a escala de cinza e o "L" branco sobre o quadrado preto, no canto de cima à esquerda, em pé;
   - o quadrado andando liso na faixa de baixo, de um lado para o outro, sem tremer;
   - a cor do quadrado trocando uma vez por segundo: em 10 segundos, 10 trocas.
4. Mude o tamanho da janela, minimize, espere alguns segundos e restaure. O cartão continua igual e no meio da janela, e o quadrado continua andando. Com a janela minimizada, o uso do processador sobe; isso já é conhecido ([TD-0015](../../docs/TECH_DEBT.md)).
5. Feche a janela. O programa termina, e o terminal mostra:
   - uma linha com `render: GPU backend vulkan on NVIDIA GeForce GTX 1060 6GB`;
   - a cada 5 segundos, uma linha `time:` com cerca de `25.0 ticks per second` e `0.00 ticks dropped`, e o tempo médio e o pior 1% dos quadros;
   - no fim, `client: the window was closed`.
6. Opcional: abra com o outro backend e confira que a janela fica igual e que o log diz `direct3d12`:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --gpu-driver direct3d12
   ```

7. Opcional: abra com um número fixo de quadros. A janela fecha sozinha depois de uns 5 segundos:

   ```powershell
   .\build\windows-dev\Games\MuOnline\App\MuClient.exe --frames 300
   ```

8. Opcional: abra a captura que o `verify` desta sessão gerou, `build\windows-dev\tests\visual\0000-test-card.png`, e confira que ela mostra o mesmo cartão que a janela, parado no tick 50.
9. No [PR #5](https://github.com/ecologycrw/nexum/pull/5), confira que os 10 checks estão verdes.

Depois, conte o que viu: se deu tudo certo, ou o que ficou diferente. Em seguida, a `/retrospectiva` roda na mesma branch, antes da sua aprovação e do merge.

## Desvios do plano

- **O relógio monotônico fica no `Engine/Time`, e não no `Engine/Platform`.** Decisão do dono em 2026-11-20, antes da T11. O plano punha o relógio na plataforma, lendo o tempo pelo SDL. A spec de referência o põe no `Engine/Time` (III.2 e IV.2), e o dono escolheu seguir a spec: o relógio lê o `std::chrono::steady_clock`, e o `Engine/Time` continua sem depender do SDL.
- **O "L" do cartão de teste fica sobre um quadrado preto de 48 pixels** (T13). A primeira faixa, que começa no canto do "L", também é branca, e sem um fundo ele não apareceria. O dono conferiu a imagem na aprovação da T14.
- **A ferramenta fica em `Tools/VisualProof`, e não em `Tools/visualproof`** (T14). A checagem `structure` exige pastas de módulo com a primeira letra maiúscula (ADR 0023). O programa continua se chamando `visualproof`.
- **O contador de pedidos de memória fica em `tests/architecture/frame_allocations/`** (T15). A checagem `cpp-rules` só aceita a troca do `operator new` global nessa pasta. O teste continua em `Games/MuOnline/Tests/FrameAllocationTest.cpp`, como o plano dizia, porque a linha vermelha proíbe nomes do MU em toda a `tests/architecture/`.
- **Escopo extra, com o "sim" do dono,** que o revisor pediu para listar aqui:
  - a P-0003 e a P-0006: o hook do início da sessão calcula, a partir do `tasks.md`, o andamento das tarefas e a etapa do ciclo;
  - a P-0007: os testes do `MuClient` e do `visualproof` sem as janelas de falha do Windows (`Nexum/Platform/FailureReports.hpp`), e um tempo limite de 300 segundos por teste;
  - o `VCPKG_APPLOCAL_DEPS=OFF` no `CMakePresets.json`, depois de duas falhas seguidas do build de jogador do Windows na CI.
- **Correções que nasceram de falhas da CI, dentro do escopo:** o desenho sem janela passou a deixar no máximo 2 quadros na fila (`OffscreenTarget::kFramesInFlight`), o que tirou os 35 pedidos de memória do macOS; e o relatório do laço passou a dar o tempo médio e o pior 1% dos quadros, que a seção "Fluidez" da spec pede.
- **Nenhum ADR novo.** As decisões da fatia aplicam ADRs que já existem (0005, 0019, 0028 e 0029) ou são detalhes da implementação, registrados no [tasks.md](tasks.md).

## Revisão

O subagente `revisor`, com contexto limpo, revisou em 2026-11-21 o diff até o commit `846efb2`, pela lista obrigatória inteira, e abriu as duas imagens da prova visual. Não achou nenhum problema crítico. Achou 2 importantes e 9 menores. As correções vão até o commit `3444b67`. O detalhe de cada achado está na seção "Revisão" do [tasks.md](tasks.md).

1. **Importante: o teste do CA-09 prova menos do que o critério diz,** porque não vê a memória que o SDL pede por `malloc`. Procede: virou o TD-0019 (M1), ligado à P-0004, e o teste ganhou um comentário sobre o limite.
2. **Importante: não havia como medir o tempo médio e o pior 1% dos quadros.** Procede: o relatório do laço passou a dar os dois, sem pedir memória, com 5 testes novos. Os números estão em "Prova visual".
3. **Menor: encerrar uma rota de log do SDL antes de uma mais nova deixava um ponteiro inválido.** Procede: corrigido, com um teste de 4 rotas.
4. **Menor: uma textura de outro depósito do mesmo dispositivo é aceita sem erro.** Procede, sem efeito hoje: virou o TD-0021 (M1).
5. **Menor: o `LoadPng` confere o tamanho só depois de ler o arquivo, e não tem fuzzing.** Procede: virou o TD-0020 (M1).
6. **Menor: o log escreve no terminal de dentro do quadro, sem fila.** Procede: entrou no TD-0011 (M7).
7. **Menor: partes do CA-06 e do CA-10 sem prova.** Procede em parte:
   - no Linux, o teste das 300 imagens passou a exigir o X11;
   - a pedido do dono, um teste novo prova a troca para o Direct3D 12 quando o Vulkan falta no Windows;
   - **não feito:** mostrar no log da CI qual backend rodou pede mudar o `ci.yml`, o `CMakePresets.json` ou o `scripts/dev.py`, que são travas. Os testes já exigem a linha do backend; falta só ela aparecer quando o teste passa.
8. **Menor: etiquetas de critério em testes que só apoiam o critério.** Procede para o CA-09 e o CA-11: as etiquetas saíram do `DrawListTest` e do `ApplicationTest`. Não procede para o CA-10 no `WindowTargetTest`, que abre uma janela nos 3 sistemas da CI.
9. **Menor: um teste que só fazia `CHECK(true)` fora do Windows.** Procede: o caso agora existe só no Windows.
10. **Menor: código repetido.** Procede: o texto do pixel, o começo do vídeo nos testes e o lado máximo de textura viraram peças únicas.
11. **Menor: documentos desatualizados.** Procede: o STATUS e o TD-0014 foram corrigidos.

## Travas alteradas

Pela comparação do `git diff --name-only main...HEAD` com a lista de travas do `AGENTS.md`:

| Trava | O que mudou | Aprovação do dono |
|---|---|---|
| `vcpkg.json` | o Tracy como a opção `profiler`; no Linux, o SDL com X11 e Wayland | com a spec, em 2026-11-20 (tabela de travas do [plano](plan.md)); feito na T02 e na T10 |
| `.github/actions/setup-build/action.yml` | os pacotes do Linux para o SDL com janela, e o Xvfb | com a spec, em 2026-11-20; T02 |
| `.github/workflows/ci.yml` | os testes do Linux dentro do Xvfb; o passo "Build with the profiler" | com a spec, em 2026-11-20; T02 e T10 |
| `tests/visual/approved/0000-test-card.png` | a primeira imagem aprovada | o próprio dono copiou a imagem, em 2026-11-21 (T14) |
| `cmake/NexumSanitizers.cmake` e `cmake/LeakSuppressions.txt` | na T03, a lista de exceções que o plano previa, com uma linha; na T13, o lavapipe carregado desde o início dos testes do build com sanitizers do Linux, o erro claro quando ele falta e a lista de exceções vazia | a primeira versão com a spec; a segunda em 2026-11-21, com três condições, todas cumpridas |
| `tests/architecture/sanitizer_canary/` | o canário de vazamento (`MemoryLeak.cpp`) | 2026-11-21 |
| `tests/architecture/frame_allocations/` e `tests/architecture/CMakeLists.txt` | o contador de pedidos de memória e, depois, a pilha de chamadas dos primeiros pedidos | 2026-11-21, com quatro condições, todas cumpridas; o diff foi mostrado antes de cada commit |
| `tests/architecture/OtherGame/` | o "outro jogo" anda pelo relógio de ticks da engine | 2026-11-21, depois de ver o diff (T16) |
| `CMakePresets.json` | o tempo limite de 300 segundos e a variável `NEXUM_FAILURES_AS_TEXT` nos presets de teste (P-0007); o `VCPKG_APPLOCAL_DEPS=OFF` | 2026-11-21, depois de ver o diff; o segundo depois de perguntar os prós, os contras e as outras soluções |
| `.claude/hooks/session-start.ps1`, `tests/hooks/session-start.tests.ps1` e as skills `/situacao`, `/revisar`, `/fechar` e `/retrospectiva` | o andamento das tarefas (P-0003) e a etapa do ciclo (P-0006) calculados pelo hook | 2026-11-21, depois de ver o diff; a P-0003 entrou pela branch `consolidacao/proposta-p-0003` |

O `.claude/settings.json` entrou por engano no commit `29fb789` e voltou ao que era no `d307a73` (ver "Erros e lições"). Ele não aparece na diferença final em relação à `main`.

## Testes removidos

Nenhum teste foi apagado nem enfraquecido. O que mudou em testes que já existiam:

- as etiquetas `[0000-CA-09]` e `[0000-CA-11]` saíram do `DrawListTest` e do `ApplicationTest`, que só apoiavam esses critérios (revisão, item 8). Os testes continuam iguais;
- o caso que só fazia `CHECK(true)` fora do Windows passou a existir só no Windows, onde ele confere algo de verdade (revisão, item 9);
- a única linha da lista de exceções de vazamento saiu, e a checagem ficou mais rígida, não mais fraca.

## Itens provisórios criados

Em [TECH_DEBT.md](../../docs/TECH_DEBT.md):

| ID | Item | Destino |
|---|---|---|
| TD-0010 | os testes do Linux abrem janelas só no X11, nunca no Wayland | M7 |
| TD-0011 | o log só vai para o terminal, de dentro do quadro e sem fila; o SDL filtra as próprias mensagens | M7 |
| TD-0012 | o build com o profiler sem preset, e sem o passo da CI no Windows | M7 |
| TD-0013 | no Windows, os argumentos chegam na página de código do sistema, e não em UTF-8 | M7 |
| TD-0014 | a tela do `MuClient` é o cartão de teste | M1 |
| TD-0015 | com a janela minimizada e o Vulkan, o laço gira sem pausa | M1 |
| TD-0016 | a prova visual só compara pixel a pixel, sem a comparação perceptual | M3 |
| TD-0017 | os samplers de cada quadro não são contados | M1 |
| TD-0018 | a CI usa placas de vídeo por software, e o piso de hardware não foi medido | M7 |
| TD-0019 | o teste do quadro não vê a memória que o SDL pede por `malloc` | M1 |
| TD-0020 | o `LoadPng` confere o tamanho só depois de ler o arquivo, e não tem fuzzing | M1 |
| TD-0021 | um handle de textura não diz de qual depósito veio | M1 |

**Resolvidos:** o TD-0003 (o SDL do Linux sem janelas de verdade) e o TD-0006 (a prova visual sem a ferramenta de comparação) saíram. O TD-0005 ficou só com o defeito do Direct3D 12, com destino M14. O destino do TD-0009 (a conta de máquina provisória) passou de M0 para M1, porque a conta `nexum-ci-dev` ainda não foi liberada pelo GitHub.

## Erros e lições

Primeiras ocorrências da escada do `docs/WORKFLOW.md`:

1. **Commit com `git add -A`** (T03, commit `29fb789`). O commit levou junto uma mudança do dono no `.claude/settings.json`, que é uma trava e que a IA não tinha visto. A mudança voltou no commit `d307a73`. Desde então, cada commit adiciona só os caminhos da tarefa.
2. **O relógio de ticks em nanossegundos inteiros, contra o ADR 0019** (T04, commit `e6179d0`). A convenção pede segundos com `double` no relógio de parede. A IA não conferiu o ADR antes de escrever. O dono escolheu seguir a regra, e o relógio foi corrigido.
3. **Um teste que só passava no Windows** (T06). O teste do assert contava com o jeito como o `std::abort` termina o programa no Windows. No Linux e no macOS, ele termina por um sinal, e a CI falhou. A lição: um teste que encerra o programa precisa da CI dos 3 sistemas antes de a tarefa fechar.
4. **Mensagens ao dono em inglês** (T14). Depois de muito código e de muitos logs em inglês, as respostas passaram para o inglês, e o dono pediu para voltar ao português.

Lições técnicas, sem erro da IA, que ficam para as próximas fatias:

- o CMake só põe o lançador dos testes, que passa as opções dos sanitizers, na frente dos programas do build, nunca na frente de um script (T13);
- um driver de vídeo descarregado antes da checagem de vazamentos faz a memória dele parecer perdida (T13);
- nos builds otimizados, o Clang pode apagar um pedido de memória que ninguém usa (T15);
- no desenho sem janela, nada limita a fila de quadros: sem um limite próprio, ela cresce quando a placa é lenta (T15).

## Propostas geradas

- **P-0003:** o andamento da fatia calculado pelo hook a partir do `tasks.md`. Aceita e aplicada nesta branch.
- **P-0004:** o contador de alocações ver também a memória que o SDL pede. Aberta.
- **P-0005:** a checagem `structure` conferir também as bibliotecas opcionais do vcpkg. Aberta.
- **P-0006:** a etapa do ciclo calculada pelo hook. Aceita e aplicada nesta branch.
- **P-0007:** os testes sem as janelas de falha do Windows, e um tempo limite geral. Aceita e aplicada nesta branch.
- **P-0008:** a prova visual do M3 com as placas de vídeo por software da CI. Aberta, para decidir antes do M3.
- **P-0009:** as opções para a CI ter uma placa de vídeo de verdade. Aberta, para rever no M3 e no M7.
- **P-0010:** a janela do `MuClient` sem a tela preta no início. Aberta, vinda do teste do dono, para o M1.

O detalhe de cada uma está em [PROPOSALS.md](../../docs/PROPOSALS.md).

**Para levar à sessão da central** (não são propostas do Nexum, porque a central é quem muda a spec de referência): a IV.12 ainda oferece um radar no canto "como opção do modo Enhanced", e o Apêndice G não lista o `light-mood.md`.
