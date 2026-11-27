# Módulo: MU, World

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

Os eventos neutros do mundo e o que o cliente sabe do mundo, montado só com eles: o herói, os objetos na visão, o inventário e as mudanças do terreno. Os eventos são o contrato estável entre o protocolo e os sistemas do jogo. A visão não conhece pacote nem rede, e nunca adivinha: um evento é a verdade (INV-01).

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `MuOnline` | Games | biblioteca do módulo MU; esta área fica em `Mu/World` |

Cabeçalhos públicos em `Games/MuOnline/Public/Mu/World/`.

## Conceitos

- **Os eventos** (`WorldEvent.hpp`): valores simples, de tamanho fixo, como `HeroLoaded`, `ObjectEntered`, `HeroStatsChanged` e os de saída da visão, de andar e de posição corrigida. A tradução os monta a partir dos pacotes, e o replay da fatia 2 vai montar os mesmos.
- **`EventQueue`** (`EventQueue.hpp`): os eventos que esperam o tick, cada um com a época da sua mensagem (`SessionEpoch`, de `Mu/Session/SessionEpoch.hpp`). O espaço é reservado uma vez. Se passar do espaço, a fila cresce em vez de descartar, porque a ordem importa. O `Drain` entrega cada evento com a época, na ordem. A visão não sabe de época.
- **`WorldView`** (`WorldView.hpp`): aplica os eventos um a um, dentro do tick. Hoje é uma tabela por id, sem entidades, porque nada é desenhado nem anda ainda. Um id desconhecido é ignorado e registrado. Cada mudança vira uma linha na categoria `world`, como "the NPC or monster of type 257 (0x0212) came into view at (130, 128)", montada no lugar, sem pedir memória.

## Como usar

No começo de cada tick, o `OnlineClient` chama o `Drain` da fila e entrega cada evento ao `WorldView::Apply` ([MU: Online](mu-online.md)). Um evento de uma época mais velha que a atual do servidor é descartado e contado antes, sem chegar à visão. O resumo do fim da sessão conta os objetos da visão (`WorldView::Objects`).

## Fontes da central

Os eventos seguem a IV.14.4 da spec de referência. O que cada evento carrega vem dos pacotes descritos na spec de protocolo, §6.11, §6.12 e §8, pela [tradução](mu-translate.md). O herói se vê sempre com o id `0x0200` (§6.5).

## Invariantes relevantes

- **INV-01:** a visão só aplica o que o servidor disse.
- **INV-09:** os eventos são aplicados um a um, na ordem de chegada, dentro do tick.
- **INV-13:** aplicar um evento não pede memória, enquanto o espaço reservado durar.

## Testes

Em `Games/MuOnline/Tests/WorldViewTest.cpp` (`muonline_tests`), só com eventos, sem rede: a entrada e a saída da visão, o andar, a correção, os stats, o inventário, o terreno, o id desconhecido e a fila de eventos, com a época de cada um.

## Itens provisórios

- [TD-0029](../TECH_DEBT.md): a visão é uma tabela por id no módulo MU, sem entidades; no M4, os objetos viram entidades do `Engine/World`.
- [TD-0037](../TECH_DEBT.md): os eventos neutros ainda não levam a época.
