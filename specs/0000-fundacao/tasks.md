# Tarefas da fatia 0

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

## Sondagem da CI (o maior risco, primeiro)

- [x] T01: Sondagem sem janela. Cria o módulo `Engine/Render` com o `GpuDevice`: a ordem de escolha do backend do ADR 0029, a opção de forçar um backend e o erro para um nome desconhecido. Também cria um alvo sem janela que limpa a imagem e lê os pixels de volta, e informa o backend e a placa. O `Engine/Platform` ganha o `VideoSystem`, que liga o vídeo do SDL (CA-06). Verificação: `py scripts/dev.py verify` passa aqui, com a sondagem rodando na placa deste computador.
  - Resultado: `verify` verde, com os sanitizers e o build de jogador. Neste computador, a sondagem usou `vulkan` na NVIDIA GeForce GTX 1060 6GB (driver 582.78), e o vídeo do SDL é o `windows`.
- [x] T02: Sondagem com janela. A `Window` mínima em `Engine/Platform` e um teste que abre a janela, limpa e apresenta alguns quadros. No Linux, o SDL com X11 e Wayland (`vcpkg.json`, TD-0003), os pacotes e o Xvfb no `setup-build`, e os testes dentro do Xvfb no `ci.yml` (CA-10). Verificação: `verify` aqui.
  - Resultado: `verify` verde. A janela mostrou os 5 quadros pelo Vulkan, e um teste novo limpa e lê de volta em cada backend que o sistema oferece: aqui, o Vulkan e o Direct3D 12 passaram na mesma placa. As três travas mudaram como o plano aprovou, e o `DEV.md` ganhou os pacotes do Linux. O Linux só é provado na T03.
- [x] T03: A sondagem na CI dos 3 sistemas, por um PR em rascunho, porque a CI só roda em PR. Verificação: todos os checks verdes, com o lavapipe no Linux, o WARP no Windows e o Metal no macOS. Se um sistema falhar, a fatia para e as opções vão para o dono.
  - O PR em rascunho é o [#5](https://github.com/ecologycrw/nexum/pull/5), aberto com o "sim" do dono. A execução [44814029499](https://github.com/ecologycrw/nexum/actions/runs/44814029499), do commit `8e5e226`, ficou verde nos 10 checks.
  - O `nexum_platform_tests` e o `nexum_render_tests` passaram em todos os jobs: Windows (MSVC), Linux (Clang e GCC) e macOS, nos builds de desenvolvimento e de jogador e com os sanitizers do Windows e do Linux. Nenhuma supressão de vazamento foi necessária.
  - No Linux, o SDL foi compilado com X11 e Wayland, e os testes rodaram dentro do Xvfb. O TD-0003 saiu dos itens provisórios. O que sobrou dele, o Wayland sem teste, virou o TD-0010, com destino M7.
  - **Falha intermitente no sanitizer do Linux:** a terceira execução ([44816192673](https://github.com/ecologycrw/nexum/actions/runs/44816192673)), do commit `d307a73`, que não mexeu em código, acusou 18 vazamentos de 128 bytes. Todos vêm de dentro do driver Vulkan, chamado pelo SDL ao procurar a placa (`VULKAN_INTERNAL_DeterminePhysicalDevice`). As duas primeiras execuções passaram com o mesmo código. A correção é a lista de exceções que o plano previa: `cmake/LeakSuppressions.txt`, com uma linha, passada aos testes pelo `cmake/NexumSanitizers.cmake` (CMake 3.29 ou mais novo). Ela vai para o report. O Windows não muda: lá, o sanitizer não procura vazamentos.
  - **O que os logs não mostram:** o nome da placa e, no Windows, qual backend começou. O CTest só imprime a saída de um teste quando ele falha. No Linux, a ordem só tem o Vulkan, e o único driver Vulkan que funciona sem placa é o lavapipe. No macOS, a ordem só tem o Metal. No Windows, pode ter sido o Vulkan ou o Direct3D 12: os dois passaram neste computador, e o teste exige que pelo menos um comece.

## Peças sem tela

- [x] T04: `Engine/Time`: o `TickClock`, com os 4 exemplos do plano como testes (CA-01, CA-02). Verificação: `verify`.
  - Resultado: `verify` verde, com os sanitizers e o build de jogador; o `nexum_time_tests` tem 10 casos e 91 conferências. O módulo novo `nexum_time` guarda o tempo em nanossegundos inteiros, então os 4 exemplos do plano dão os valores exatos de α, e os 10 minutos simulados dão exatamente 15.000 ticks, a posição 45.000 e α 0. Os quadros desse teste vão de 5 a 200 ms, quase todos com frações de milissegundo, numa ordem irregular e fixa, sem sorteio do sistema.
  - Também testados: a pausa (nenhum tick, α parado e o tempo pausado descartado, sem contar como atraso), o avanço tick a tick (um tick por pedido, no máximo 5 por quadro), um pedido de avanço sem pausa (não muda nada), a volta da pausa (esquece os pedidos que não rodaram) e um tempo de quadro negativo (conta como zero).
  - O `ARCHITECTURE` ainda lista o `Engine/Time` e o `Engine/Render` entre as pastas que não existem. A correção fica na T16, que atualiza a documentação viva.
  - **Correção depois do commit `e6179d0`:** o relógio recebia o tempo do quadro em nanossegundos inteiros, o que desviava do [ADR 0019](../../docs/adr/0019-convencoes.md) (relógio de parede em segundos com `double`). O dono escolheu seguir a regra: o relógio recebe e devolve segundos com `double` (o tipo `Seconds`) e, por dentro, soma nanossegundos inteiros. O dono pediu também que a conversão arredonde, nunca corte, e que os testes passem o tempo em segundos com casas decimais. Os testes agora passam 0,016, 0,039, 0,25 e os quadros dos 10 minutos em segundos. Um teste novo mostra que 0,007817 s, guardado como `double`, fica um pouco abaixo de 7.817.000 ns. Prova de que os testes pegam o erro: trocando por um instante o arredondamento por um corte, os 10 minutos deram 14.999 ticks, a posição 44.997 e α 0,999997, e o teste do arredondamento também falhou. Um tempo de quadro que não é número, ou infinito, agora também conta como zero. São 11 casos de teste, e o `verify` ficou verde.
- [x] T05: Os temporizadores de parede e de tick e o diagnóstico do laço: ticks por segundo, ticks descartados, maior quadro e histograma do tick (CA-03). Verificação: `verify`.
  - Resultado: `verify` verde, com os sanitizers e o build de jogador. O `nexum_time_tests` passou a ter 25 casos e 302 conferências, com os testes novos em `TimersTest.cpp` e `TickDiagnosticsTest.cpp`, como o plano previa.
  - `WallTimer`: dispara a cada período, em segundos do relógio monotônico, a primeira vez um período depois do início. Depois de uma parada longa (um mapa que demorou para carregar), dispara uma vez só e continua no mesmo ritmo. Ele não sabe nada de ticks: com a simulação pausada, 10 s de quadros dispararam um temporizador de 5 s duas vezes.
  - `TickTimer`: conta só ticks. Pausado, 2 s de quadros não o mexeram. Com 25 pedidos de avanço, ele disparou em 0,08 s de relógio. Rodando, disparou no 25º tick, como o quadrado da tela da fundação vai usar.
  - `TickDiagnostics`: soma o tempo de parede, os ticks, os ticks descartados (o tempo descartado medido em ticks: 50 ms são 1,25 tick) e o maior quadro. Ele também monta o histograma do tempo de cada tick, nas faixas até 1, 2, 4 e 8 ms e acima de 8 ms. Os 8 ms vêm do D-06 da spec: é o limite (20% dos 40 ms) a partir do qual a simulação pode precisar de uma thread própria. Cada relatório cobre só o que veio depois do anterior.
  - Os tempos seguem a decisão do dono na T04: segundos com `double`. Tempos negativos, que não são número ou infinitos contam como zero, pela mesma função usada no relógio de ticks (`Private/UsableDuration.hpp`). Um temporizador com período zero, negativo ou que não é número nunca dispara.
- [x] T06: O `NEXUM_ASSERT`, ligado no build de desenvolvimento e desligado no de jogador, com um teste do CTest que espera a falha e a mensagem (CA-04). Verificação: `verify`.
  - Resultado: `verify` verde nos três builds (desenvolvimento, sanitizers e jogador). O `NEXUM_ASSERT` fica em `Nexum/Core/Assert.hpp`. Quem decide se ele está ligado é a definição pública `NEXUM_ASSERTS_ENABLED`, que segue o `NEXUM_AUTHORING` do preset. Com a condição falsa, ele escreve `NEXUM_ASSERT failed: <condição> at <arquivo>:<linha>` na saída de erro e encerra o programa com `std::abort`. No build de jogador, a condição nem é avaliada.
  - O teste do CTest é um programa próprio (`nexum_core_assert_failure_tests`), porque o assert encerra o programa inteiro. Ele usa o `main` comum dos testes, que já impede a janela de aviso do Windows ao encerrar. No build de desenvolvimento, o teste exige a mensagem com a condição, o arquivo e a linha, e falha se o programa continuar. No build de jogador, ele exige que o programa continue. A saída real no Windows foi `NEXUM_ASSERT failed: cells + 1 == 4 at ...\Engine\Core\Tests\AssertFailureTest.cpp:13`, seguida do encerramento (SIGABRT), e no build de jogador saiu `the program went on after a false assert`. Um teste no `nexum_core_tests` confere que a condição roda uma vez no desenvolvimento e nenhuma no jogador.
  - Avaliação pedida na T05: os temporizadores continuam sem assert para período inválido. Hoje esse caso tem um comportamento definido (o temporizador nunca dispara) e testado em todos os builds. Com um assert, esse teste não poderia rodar no build de desenvolvimento, e um temporizador que nunca dispara já é fácil de notar.
- [x] T07: O `Handle` geracional e o `HandlePool` de capacidade fixa (CA-04). Verificação: `verify`.
  - Resultado: `verify` verde nos três builds. O `nexum_core_tests` passou a ter 16 casos e 123 conferências, com os novos em `HandlePoolTest.cpp`. O `Handle<T>` (em `Nexum/Core/Handle.hpp`) guarda o lugar e a geração, e o tipo do objeto faz parte dele: um handle de textura não serve como handle de voz. O handle padrão não aponta para nada, porque as gerações começam em 1.
  - O `HandlePool<T>` (em `Nexum/Core/HandlePool.hpp`) reserva toda a memória ao ser criado e nunca cresce. Os testes provam três coisas. Um handle de objeto removido é recusado, mesmo com o lugar reaproveitado por outro objeto: o teste confere que o lugar é o mesmo e a geração mudou. Um pool cheio devolve o erro `the pool is full (2 objects)` e continua com a mesma capacidade. E os objetos não mudam de endereço enquanto outros entram e saem.
  - O III.6 da spec manda os pools de partículas, efeitos e rastros descartarem o mais velho quando enchem. Essa política fica para quem usa o pool, por cima do erro, quando os efeitos chegarem (M5).
  - Um lugar cuja geração chegaria ao fim (depois de cerca de 4 bilhões de reusos) é aposentado, para que nenhum handle antigo volte a valer. Esse caminho não tem teste, porque o teste precisaria de 4 bilhões de remoções.
- [ ] T08: A `FrameArena` e a `Image` RGBA (CA-04). Verificação: `verify`.
- [ ] T09: `Engine/Debug`: o `Logger` com categorias, níveis e saída trocável, e as mensagens do SDL passando pelo log (CA-05). Verificação: `verify`.
- [ ] T10: O profiler: as macros, o Tracy com `NEXUM_PROFILER`, a recusa no build de jogador e um passo da CI que compila com ele ligado (CA-05). Verificação: `verify` e o build com o profiler ligado aqui.

## Parte gráfica

- [ ] T11: Plataforma: a entrada pelos *main callbacks* do SDL, os eventos da janela (redimensionar, minimizar, restaurar e fechar), o relógio monotônico e a leitura e a gravação de PNG (CA-10, CA-11). Verificação: `verify`.
- [ ] T12: Render: texturas por handle, o envio de uma textura gerada, a cópia para a tela e a leitura do alvo sem janela como `Image` (CA-07). Verificação: `verify`.
- [ ] T13: A tela da fundação no `MuClient`: o cartão de teste, o quadrado que anda por tick com interpolação, o retrato na arena do quadro, `--frames`, `--gpu-driver` e o log do backend e da placa (CA-06, CA-10, CA-11). Verificação: `verify` e a janela aberta aqui.
- [ ] T14: A prova visual: `--capture`, a ferramenta `Tools/visualproof` e o teste do CTest que junta os dois. A primeira imagem vai para a aprovação do dono antes de entrar em `tests/visual/approved/` (CA-07, CA-08). Verificação: `verify` e a imagem conferida.
- [ ] T15: O teste de alocação do quadro: 1.000 quadros sem pedir memória ao sistema (CA-09). Verificação: `verify`.

## Fechamento

- [ ] T16: O `other_game` passa a usar o relógio de ticks. A documentação viva fica em dia: VISÃO, VISUAL, ARCHITECTURE, CONVENTIONS, THIRD_PARTY, DEV.md e os itens provisórios TD-0005 e TD-0006 (CA-12, CA-13). Verificação: `verify` e a checagem `docs`.
- [ ] T17: A CI completa verde nos 3 sistemas (CA-10, CA-12). Verificação: os checks do PR.

O CA-11 é a verificação manual do dono, registrada no report.

## Bloqueios

*(nenhum)*

## Prova visual

O que a IA viu em cada imagem da cena de prova, inclusive o que ficou ruim.

## Revisão

Achados do revisor e o que foi feito com cada um. Preenchido pelo `/revisar`.

## Notas para a próxima sessão

- O PR #5 está aberto só para a CI rodar a cada envio; ele não está pronto para revisão.
- O aviso automático da CI (Auto-fix) no PR #5 vale só para a sessão que o ligou. Numa sessão nova, ligue de novo: a permissão `mcp__ccd_pr__set_monitor` está no `.claude/settings.local.json` deste computador.
- O `CMakePresets.json` tira dos testes os que têm a etiqueta `gpu`. A fatia não usa essa etiqueta: o plano manda os testes da placa de vídeo rodarem em todo `verify` e na CI, que usa placas por software. Mexer no filtro é mudar uma trava.
- No commit `29fb789` (T03), o `git add -A` levou junto uma mudança do dono no `.claude/settings.json`, que é uma trava. A IA não tinha visto a mudança. Daqui em diante, cada commit adiciona só os caminhos da tarefa, nunca `-A`. Esta é a primeira ocorrência, para o report.
- O `guard-bash` recusa um comando que cita um arquivo de trava, como `scripts/dev.py`, e também grava um arquivo (`Out-File`, `>`). Para resumir a saída do `verify`, filtre com `Select-String`, sem gravar arquivo.
- **Tempo do relógio de parede:** em segundos com `double` (`nexum::time::Seconds`), como manda o [ADR 0019](../../docs/adr/0019-convencoes.md). Quem precisa de somas exatas converte para nanossegundos inteiros, sempre arredondando, nunca cortando (decisão do dono na T04). Os temporizadores e o diagnóstico da T05 seguem a mesma regra.
- **Para a T11:** a III.2 da spec põe o relógio monotônico no `Engine/Time`, e o plano o pôs no `Engine/Platform`, lendo pelo SDL, sem registrar o motivo. Antes da T11, explicar as duas opções ao dono. A recomendação é seguir a spec, com o relógio padrão do C++ dentro do `Engine/Time`.
