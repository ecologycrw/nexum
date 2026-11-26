# Módulo: MU, Online

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O que liga as peças da rede do MU ao quadro do `MuClient`: o transporte de verdade, que abre uma conexão para cada endereço que a sessão pede, e o cliente online, que roda a [sessão](mu-session.md), a [tradução](mu-translate.md) e a [visão do mundo](mu-world.md), cada uma no seu passo do quadro. Também ficam aqui as opções da linha de comando do `MuClient` (`Mu/App`). O cliente online não lê relógio: cada chamada traz a hora do quadro.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/Online` e `Mu/App` |
| `MuClient` | Games | o programa, em `Games/MuOnline/App` |

Cabeçalhos públicos em `Games/MuOnline/Public/Mu/Online/` e `Games/MuOnline/Public/Mu/App/ClientOptions.hpp`.

## Conceitos

- **`Transport`** (`Transport.hpp`): o que o cliente online precisa da rede: a `SessionLink` da sessão e um jeito de entregar à sessão os eventos que chegaram (`Deliver`). Os testes trazem o seu, com as mensagens na memória.
- **`NetworkTransport`** (`NetworkTransport.hpp`): o transporte de verdade. Abre uma `Connection` do [Net](net.md) para cada endereço, cada uma com um `ClientCodec` novo, e guarda a época de cada uma. Só uma conexão fica aberta por vez.
- **`OnlineClient`** (`OnlineClient.hpp`): no passo 2 do quadro (`TakeNetwork`), tira os eventos da rede e traduz as mensagens do mundo, sem aplicar. No passo 3 (`AdvanceClock`), roda o relógio da sessão. No começo de cada tick, aplica os eventos (`ApplyEvents`). O `Status` diz se a sessão caiu ou se o cliente já ficou no mundo o tempo pedido. O `Summary` é a última linha do log, como "the session ends: 180 s in the world, pings sent: 9, objects in view: 6".
- **As opções do `MuClient`** (`ClientOptions.hpp`): `--connect HOST:PORT` (o Connect Server; vem com `--account`), `--account NOME`, `--server ID` e `--character NOME` (sem eles, o primeiro da lista) e `--world-seconds N` (encerra com sucesso depois de N segundos no mundo). A senha nunca vem pela linha de comando: o `main` a lê com o `ReadHiddenLine` do [Platform](platform.md), sem eco. Uma senha vazia ou com mais de 20 bytes é recusada antes de qualquer conexão.

## Como usar

O `FoundationApp` (`Games/MuOnline/App/Private/FoundationApp.cpp`) guarda o `NetworkTransport` e o `OnlineClient` e os liga à tela com o `FoundationScreen::Attach`. Para entrar num OpenMU local, siga o [DEV.md](../setup/DEV.md#o-servidor-de-teste-do-openmu).

## Fontes da central

O cliente online não reproduz nada novo: ele só põe a sessão, a tradução e a visão no quadro da [ARCHITECTURE](../ARCHITECTURE.md#o-quadro). As fontes de cada peça estão nas páginas delas. O servidor de teste segue a spec de protocolo, §10, e o achado `2026-11-21-versao-docker-para-testes.md`: a imagem 0.10.1 fixada pela impressão digital, o `-demo`, o resolvedor de IP e as contas de teste.

## Invariantes relevantes

- **INV-06:** o quadro nunca espera pela rede: o transporte só tira e põe nas filas.
- **INV-04 e a ordem do quadro:** os eventos são aplicados no tick, antes dos sistemas; o desenho só lê.
- **INV-13:** o quadro com mensagens de rede chegando não pede memória (CA-11 da fatia 1).
- **CA-12 da fatia 1:** a senha não passa pela linha de comando, pelo disco nem pelo log.

## Testes

- `Games/MuOnline/Tests/OnlineClientTest.cpp`, com um transporte de teste, `NetworkTransportTest.cpp`, com o servidor local do [Net](net.md), e `ClientOptionsTest.cpp`.
- `Games/MuOnline/Tests/FrameAllocationTest.cpp` (`muonline_frame_allocation_tests`): 1.000 quadros com 5 mensagens do mundo em cada um, sem pedir memória.
- Os testes do `MuClient` em `Games/MuOnline/App/CMakeLists.txt`: a senha lida da entrada, a senha longa demais e a recusa de `--password`.
- **Contra um OpenMU de verdade** (`tests/integration/openmu`, etiqueta `openmu`): 3 minutos no mundo com a conta `test0` e a senha errada com a `test1`. Os dois só rodam quando a variável `NEXUM_OPENMU_ADDRESS` diz o endereço do Connect Server; sem ela, como na CI, o CTest os marca como pulados.

## Itens provisórios

- [TD-0032](../TECH_DEBT.md): a conta, o servidor e o personagem vêm da linha de comando, e a senha do terminal, até a tela de login do M6.
- [TD-0033](../TECH_DEBT.md): o mapa é dado por pronto assim que os dados do personagem chegam.
- [TD-0034](../TECH_DEBT.md): o teste de integração roda só no computador do dono.
