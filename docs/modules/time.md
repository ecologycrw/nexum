# Módulo: Time

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O tempo da engine: o relógio monotônico, o laço de ticks fixos de 40 ms, os temporizadores de parede e de tick, e o diagnóstico do laço. O relógio de ticks não lê relógio nenhum: quem o usa entrega o tempo de cada quadro, e por isso ele é testado com tempos exatos. O módulo não depende do SDL.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `nexum_time` (`Nexum::Time`) | Engine | biblioteca; sem dependências, só o `std::chrono` |

Cabeçalhos públicos em `Engine/Time/Public/Nexum/Time/`.

## Conceitos

- **`Seconds`** (`Seconds.hpp`): o tempo de parede em segundos, com `double` ([ADR 0019](../adr/0019-convencoes.md)). Por dentro, as somas usam nanossegundos inteiros, sempre arredondando, nunca cortando.
- **`MonotonicClock`** (`MonotonicClock.hpp`): lê o `std::chrono::steady_clock`, que nunca volta. O `Elapsed` dá o tempo desde o início, e o `Lap` dá o tempo desde a volta anterior, que é o tempo do quadro.
- **`TickClock`** (`TickClock.hpp`): o laço do [ADR 0005](../adr/0005-tick-fixo-de-25-hz.md).
  - `AdvanceFrame(tempo do quadro)` diz quantos ticks rodar e quanto tempo foi descartado.
  - No máximo 5 ticks por quadro (`kMaxTicksPerFrame`); o tempo acima disso é descartado e contado, nunca vira ticks depois.
  - `Alpha()` é o resto do acumulador dividido por 40 ms (`kTickDuration`), para o desenho ficar entre dois ticks.
  - `Pause` e `Resume`: pausado, não roda nenhum tick, o α fica parado e o tempo pausado é descartado. Essa é uma decisão da engine, não do original.
  - `RequestStep`: com a simulação pausada, roda exatamente um tick por pedido.
  - Um tempo negativo, que não é número ou infinito conta como zero.
- **`WallTimer`** (`WallTimer.hpp`): dispara a cada período de tempo de parede, também com a simulação pausada. Depois de uma parada longa, dispara uma vez só e continua no mesmo ritmo.
- **`TickTimer`** (`TickTimer.hpp`): dispara a cada N ticks e não sabe nada do tempo de parede.
- **`TickDiagnostics`** (`TickDiagnostics.hpp`): soma os quadros e os ticks e entrega o relatório (`TickReport`): ticks por segundo, ticks descartados, número de quadros, tempo médio, pior 1% e maior quadro, e o histograma do tempo de cada tick nas faixas até 1, 2, 4 e 8 ms e acima de 8 ms. Cada relatório cobre só o que veio depois do anterior, e nada pede memória.

## Como usar

A tela da fundação (`Games/MuOnline/Private/Foundation/FoundationScreen.cpp`) segue a ordem do quadro do III.4 da spec: relógios de parede, ticks e desenho com o α. O "outro jogo" dos testes de arquitetura (`tests/architecture/OtherGame/`) usa o mesmo relógio, sem nada do MU.

## Fontes da central

- O ritmo de 25 ticks por segundo vem do original: I.2 e D-05 da [spec de referência](../referencia/README.md).
- As regras do laço (até 5 ticks por quadro, o descarte, o α e os relógios de parede) são decisões da spec: D-05, III.4 e IV.2. A faixa de 8 ms do histograma vem da D-06.
- As fontes ficam registradas no [plano da fatia 0](../../specs/0000-fundacao/plan.md).

## Invariantes relevantes

- **INV-05:** tick fixo de 40 ms e relógios monotônicos. O teste de 10 minutos simulados prova o ritmo exato.
- **INV-13:** o diagnóstico e o relatório não pedem memória.
- **INV-01:** o laço só move a apresentação; o servidor continua mandando.

## Testes

Em `Engine/Time/Tests/` (`nexum_time_tests`): `TickClockTest.cpp` (o teste de tempo da spec e os exemplos do plano), `TimersTest.cpp`, `TickDiagnosticsTest.cpp` e `MonotonicClockTest.cpp`.

## Itens provisórios

Nenhum.
