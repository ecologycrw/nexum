# Módulo: MU, Online

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O que liga o servidor ao quadro do `MuClient`. O cliente online roda a fila de eventos e a [visão do mundo](mu-world.md), e fala com o servidor só por um contrato, o adaptador de servidor ([ADR 0035](../adr/0035-adaptador-de-servidor-gravacao-e-replay.md)). Esse contrato tem duas implementações: o OpenMU ao vivo, que junta o transporte, a [sessão](mu-session.md) e a [tradução](mu-translate.md), e o replay de uma [gravação](mu-recording.md). Também ficam aqui o transporte de verdade e as opções da linha de comando do `MuClient` (`Mu/App`). Nada aqui lê relógio: cada chamada traz a hora do quadro.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/Online` e `Mu/App` |
| `MuClient` | Games | o programa, em `Games/MuOnline/App` |

Cabeçalhos públicos em `Games/MuOnline/Public/Mu/Online/` e `Games/MuOnline/Public/Mu/App/ClientOptions.hpp`.

## Conceitos

- **`ServerAdapter`** (`ServerAdapter.hpp`): o contrato entre o cliente online e o servidor.
  - `Start(now)` começa a sessão.
  - `TakeEvents(now, fila)`, no passo 2 do quadro, põe na fila os eventos que chegaram, cada um com a época, sem nunca esperar pela rede ou pelo disco.
  - `AdvanceClock(now)`, no passo 3, roda os relógios de parede do servidor.
  - `EventsApplied(visão, quantos, now)` é o aviso, depois de cada tick que tirou eventos da fila, de quantos eram (aplicados ou descartados) e de como a visão ficou.
  - `CurrentEpoch()`, `Status()` e `Summary(now)` dão a época atual, o estado e as palavras do servidor para a última linha.

  O estado é um de quatro: conectando, no mundo, terminou ou falhou, este com o motivo. O contrato não sabe de bytes, de conexões nem de arquivos, e nunca vê a conta nem a senha.
- **`LiveServer`** (`LiveServer.hpp`): o OpenMU 106.3 ao vivo. Recebe as opções da sessão (com a conta e a senha) e o `Transport` quando nasce. Entrega os eventos da rede à sessão, entra no mundo quando os dados do personagem chegam e traduz as mensagens do mundo. Com um `Recorder` ([MU: Recording](mu-recording.md)), avisa o gravador da entrada no mundo, de cada mensagem do mundo antes da tradução, de cada mudança de época e de cada tick. O `Summary` dá os pings enviados.
- **`ReplayServer`** (`ReplayServer.hpp`): o replay de uma gravação já lida e conferida, que precisa viver mais que ele. Os registros vêm em grupos, cada um terminado pela entrada no mundo, por um tick ou pelo fim. Cada grupo é entregue no passo 2 do primeiro quadro em que chegou a hora do seu último registro, contada desde o `Start`: as mensagens vão para a mesma tradução, e as mudanças de época mudam a época atual. Depois de um grupo com tick, o replay espera o tick seguinte do cliente. Nesse tick, ele confere a quantidade de eventos e, quando o registro seguinte é um ponto de controle, a impressão do estado, os objetos e os itens. Só então entrega o próximo grupo. Assim, o agrupamento por tick é o da sessão gravada, seja qual for o ritmo dos quadros.
  - Uma diferença vira o estado "falhou", com a mensagem no log (categoria `replay`), como "the tick 2 at 340000 us took N events in the recording, but M in the replay".
  - O fim da gravação vira o estado "terminou": "the replay reached the end of the recording, where the client ended it: every tick and checkpoint matched".
  - O `Summary` dá "the replay checked N messages, N ticks and N checkpoints". Os ticks depois do fim não são conferidos. Entregar e conferir os registros não pede memória: só uma diferença monta a sua mensagem, uma vez.
- **`Transport`** (`Transport.hpp`): o que o `LiveServer` precisa da rede: a `SessionLink` da sessão e um jeito de entregar à sessão os eventos que chegaram (`Deliver`). Os testes trazem o seu, com as mensagens na memória (`Tests/ScriptedTransport.hpp`).
- **`NetworkTransport`** (`NetworkTransport.hpp`): o transporte de verdade. Abre uma `Connection` do [Net](net.md) para cada endereço, cada uma com um `ClientCodec` novo, e guarda a época de cada uma. Só uma conexão fica aberta por vez.
- **`OnlineClient`** (`OnlineClient.hpp`): fica só com o que é do jogo: a fila de eventos, a visão do mundo e o tempo pedido no mundo. No passo 2 do quadro (`TakeEvents`), tira os eventos do servidor, sem aplicar. No passo 3 (`AdvanceClock`), roda os relógios do servidor. No começo de cada tick (`ApplyEvents`), aplica os eventos em ordem. Antes disso, descarta e conta cada evento de uma época mais velha que a `CurrentEpoch()` do servidor, e o log registra o descarte uma vez por tick. Quando o tick tirou algum evento, o cliente avisa o servidor. O `Status` diz se o servidor falhou ou terminou, ou se o cliente já ficou no mundo o tempo pedido. O `Summary` é a última linha do log, como "the session ends: 180 s in the world, pings sent: 9, objects in view: 6".
- **As opções do `MuClient`** (`ClientOptions.hpp`):
  - `--connect HOST:PORT` (o Connect Server; vem com `--account`), `--account NOME`, `--server ID` e `--character NOME` (sem eles, o primeiro da lista) e `--world-seconds N` (encerra com sucesso depois de N segundos no mundo);
  - só no build de desenvolvimento: `--record ARQUIVO` (grava a sessão no mundo; vem com `--connect`), `--server-version TEXTO` (de 1 a 64 bytes, para o cabeçalho; vem com `--record`) e `--replay ARQUIVO` (reproduz sem servidor; não vem com as opções do caminho até o mundo, nem com `--record` ou `--capture`). O build de jogador recusa as três, como recusa o `--capture`.

  A senha nunca vem pela linha de comando: o `main` a lê com o `ReadHiddenLine` do [Platform](platform.md), sem eco. Uma senha vazia ou com mais de 20 bytes é recusada antes de qualquer conexão.

## Como usar

O `FoundationApp` (`Games/MuOnline/App/Private/FoundationApp.cpp`) monta o servidor (o `LiveServer` com o `NetworkTransport`, ou o `ReplayServer` que o `RecordingTools` dá), guarda o `OnlineClient` e o liga à tela com o `FoundationScreen::Attach`. No fim, ele aplica os eventos que ainda esperam um tick e termina a gravação. A ligação com o `--record` e o `--replay` fica em `Games/MuOnline/App/Private/RecordingTools.cpp`, compilado só com `NEXUM_AUTHORING`: antes da senha, ele lê a gravação do `--replay` ou escreve o cabeçalho do `--record`, e, depois do último quadro, escreve o arquivo e dá a última linha, como "client: wrote the recording to PATH: 23 messages, 6 ticks and 2 checkpoints". Para entrar num OpenMU local, gravar e reproduzir, siga o [DEV.md](../setup/DEV.md#o-servidor-de-teste-do-openmu).

## Fontes da central

O cliente online e o replay não reproduzem nada novo: eles só põem a sessão, a tradução e a visão no quadro da [ARCHITECTURE](../ARCHITECTURE.md#o-quadro), e o replay é do próprio Nexum ([MU: Recording](mu-recording.md)). As fontes de cada peça estão nas páginas delas. O servidor de teste segue a spec de protocolo, §10, e o achado `2026-11-21-versao-docker-para-testes.md`: a imagem 0.10.1 fixada pela impressão digital, o `-demo`, o resolvedor de IP e as contas de teste.

## Invariantes relevantes

- **INV-06:** o quadro nunca espera pela rede nem pelo disco: o transporte só tira e põe nas filas, e a gravação é lida antes da janela e escrita depois do último quadro.
- **INV-04 e a ordem do quadro:** os eventos são aplicados no tick, antes dos sistemas; o desenho só lê.
- **INV-09:** os eventos são aplicados em ordem, com a época, e os de uma época velha são descartados antes de chegar à visão.
- **INV-13:** o quadro com mensagens de rede chegando, gravando ou reproduzindo não pede memória (CA-11 da fatia 1 e CA-10 da fatia 2).
- **CA-12 da fatia 1:** a senha não passa pela linha de comando, pelo disco nem pelo log.

## Testes

- Em `Games/MuOnline/Tests/` (`muonline_tests`): `OnlineClientTest.cpp` (o cliente com o `LiveServer` e um transporte de teste, um adaptador de teste que prova que o cliente aplica os eventos em ordem sem saber de onde vêm, e o descarte pela época), `ReplayServerTest.cpp` (o ritmo, os grupos, as diferenças com a mensagem certa, o fim, a repetição e a gravação de referência), `NetworkTransportTest.cpp`, com o servidor local do [Net](net.md), e `ClientOptionsTest.cpp`.
- `Games/MuOnline/Tests/FrameAllocationTest.cpp` (`muonline_frame_allocation_tests`): 1.000 quadros com 5 mensagens do mundo em cada um, gravando e reproduzindo, sem pedir memória.
- Os testes do `MuClient` em `Games/MuOnline/App/CMakeLists.txt`: a senha lida da entrada, a senha longa demais, a recusa de `--password`, a gravação de uma sessão que não chega ao mundo e o replay dela, o replay de duas sementes do fuzzing e o da gravação de referência, as recusas de um caminho que não dá para escrever e de um arquivo que não é uma gravação, e, no build de jogador, a recusa das três opções.
- **A gravação de referência** (`Games/MuOnline/Tests/Recordings/openmu-two-players.mucap`): cerca de 20 s no OpenMU 0.10.1 local, com a `test1Dk` entrando na visão e saindo. É reproduzida nos 3 sistemas, num teste da biblioteca, sem relógio de parede, e no `muclient_replays_the_reference_recording`, no ritmo gravado. Para refazê-la, veja o [DEV.md](../setup/DEV.md#refazer-a-gravação-de-referência).
- **Contra um OpenMU de verdade** (`tests/integration/openmu`, etiqueta `openmu`): 3 minutos no mundo com a conta `test0`, a senha errada com a `test1`, e o `openmu_muclient_records_and_replays`, que grava 20 s com a `test0` e reproduz sem servidor, com as mesmas contagens, as mesmas linhas do mundo e a conta fora do arquivo. Os três só rodam quando a variável `NEXUM_OPENMU_ADDRESS` diz o endereço do Connect Server; sem ela, como na CI, o CTest os marca como pulados.

## Itens provisórios

- [TD-0032](../TECH_DEBT.md): a conta, o servidor e o personagem vêm da linha de comando, e a senha do terminal, até a tela de login do M6.
- [TD-0033](../TECH_DEBT.md): o mapa é dado por pronto assim que os dados do personagem chegam.
- [TD-0034](../TECH_DEBT.md): o teste de integração roda só no computador do dono.
- [TD-0038](../TECH_DEBT.md): o adaptador de servidor ainda não tem os comandos do jogador.
