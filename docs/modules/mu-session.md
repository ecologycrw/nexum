# Módulo: MU, Session

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A sessão do cliente 106.3 com o OpenMU, de "Desconectado" a "No mundo": a ordem da conversa, os prazos de cada resposta, a época de cada conexão, o ping e as mensagens do mundo guardadas até o tick. A sessão não tem conexão nem relógio próprios: ela recebe os eventos e a hora do quadro, e manda o que precisa por uma interface. Assim, os testes a alimentam sem rede. Ela não traduz o mundo (isso é da [tradução](mu-translate.md)) e, nesta fatia, não reconecta.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/Session` |

Cabeçalhos públicos em `Games/MuOnline/Public/Mu/Session/`.

## Conceitos

- **`Session`** (`Session.hpp`): a máquina de estados.
  - Os estados são `Disconnected`, `ConnectServer`, `ConnectingGameServer`, `Login`, `Characters`, `Loading` e `InWorld`. O `ChangingMap` (M4) e o `Reconnecting` (M7) já têm nome, mas nada chega a eles ainda.
  - O `Handle(época, evento, agora)` recebe cada evento de uma conexão. Um evento de uma época velha é descartado.
  - O `AdvanceClock(agora)`, chamado a cada quadro, cuida dos prazos e do ping.
  - O `FinishLoading()` diz que o mapa está pronto e leva a sessão de "Carregando" a "No mundo".
  - O `TakeWorldMessages` entrega as mensagens do mundo que esperam, na ordem, primeiro os dados do personagem.
  - O `Epoch()` é a época atual, que o `LiveServer` dá como a época do servidor.
  - Cada resposta esperada tem um prazo de 10 segundos, e um prazo vencido diz o que faltou. Uma recusa de login diz o significado do código (por exemplo, "invalid password") e encerra.
- **`SessionLink`** (`Session.hpp`): o que a sessão pede ao app: `Open` (abrir uma conexão com um endereço e uma cifra), `Send` e `Close`. A cada `Open`, a época aumenta.
- **`SessionOptions`**: a conta, a senha, o servidor e o personagem escolhidos. Sem servidor ou personagem, vale o primeiro da lista. A sessão recebe a senha como valor, sem saber de onde ela veio.
- **`SessionEpoch`** (`SessionEpoch.hpp`): o número da época, um valor à parte, com a ordem (um número menor é uma época mais velha). Cada conexão nova recebe o seguinte. A fila de eventos do [MU: World](mu-world.md) e a [gravação](mu-recording.md) guardam a época sem conhecer a sessão.
- **`MessageQueue`** (`MessageQueue.hpp`): as mensagens do mundo que esperam a tradução, cada uma com a época da conexão. O espaço é reservado uma vez. Se passar do espaço, a fila cresce em vez de descartar, porque a ordem importa.
- **O ping** (`Ping.hpp`): o `PingClock` manda o `C3 0E 00` a cada 20 s de relógio de parede, desde a entrada no Game Server, como o cliente real. O `WritePing` leva o tempo em ms e as velocidades de ataque e de magia conhecidas (0 e 0 até o personagem entrar).
- **O log:** cada passo vira uma linha na categoria `session`, como "the login was accepted". O log de pacotes (`session.packets`) esconde o usuário e a senha.

## Como usar

O `LiveServer` ([MU: Online](mu-online.md)) segura a sessão. No passo 2 do quadro, ele entrega os eventos da rede ao `Handle`, chama o `FinishLoading` quando os dados do personagem chegam e tira as mensagens do mundo com o `TakeWorldMessages`, para a tradução. O passo 3 chama o `AdvanceClock`. Os testes usam mensagens em memória e um relógio de teste (`Games/MuOnline/Tests/SessionTest.cpp`, com as mensagens em `SessionMessages.hpp`).

## Fontes da central

- A ordem da conversa e cada campo: a spec de protocolo, §6.1 a §6.12, e o achado `2026-11-21-sessao-106-3-campo-a-campo.md`.
- Os estados e os erros: a §9 e a IV.14.1 da spec de referência.
- O ping: a §8, e o achado `2026-11-21-capturas-reais-sessao-106-3.md`, §4.1 (o ping desde a entrada no Game Server).
- As portas e o Connect Server: a §2.

Os prazos de 10 segundos são do Nexum: o cliente original não tem prazo e trava (a recomendação 5 da spec de protocolo).

## Invariantes relevantes

- **INV-01:** a sessão só segue o que o servidor diz e sempre o endereço que o Connect Server devolve.
- **INV-05:** os prazos e o ping usam o relógio de parede monotônico do quadro, nunca os ticks.
- **INV-09:** a época descarta os eventos de uma conexão velha, e tudo sai pela única fila de saída.
- **INV-12:** cada resposta é lida e conferida antes de mudar o estado; dados do personagem com menos de 92 bytes são um erro.
- **CA-12 da fatia 1:** a senha nunca vai para o log.

## Testes

Em `Games/MuOnline/Tests/` (`muonline_tests`): `SessionTest.cpp` (de "Desconectado" a "No mundo", as recusas, os prazos e a época), `MessageQueueTest.cpp` e `PingTest.cpp` (os 20 s com um relógio de teste e quadros irregulares). Contra um OpenMU de verdade, o teste de integração de `tests/integration/openmu` ([MU: Online](mu-online.md)).

## Itens provisórios

- [TD-0027](../TECH_DEBT.md): os prazos são fixos, de 10 s.
- [TD-0028](../TECH_DEBT.md): uma queda ou uma recusa encerra a sessão, sem reconexão; e a conta e a senha são apagadas depois do primeiro login.
- [TD-0036](../TECH_DEBT.md): cópias do login montado ficam em outros buffers até serem sobrescritas.
